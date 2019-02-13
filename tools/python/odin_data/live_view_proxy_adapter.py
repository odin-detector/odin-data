"""ODIN data live view proxy adapter.

This module implements an odin-control adapter capable of subscribing to
multiple Odin Data Live View plugins and combining all streams of frames
into a single ZMQ stream.

Created on 28th January 2019

:author: Adam Neaves, STFC Application Engineering Group
"""
import logging
from queue import PriorityQueue
from queue import Full as QueueFullException
from queue import Empty as QueueEmptyException

import time
from concurrent import futures
from tornado.escape import json_decode, json_encode
from tornado.ioloop import IOLoop
from tornado.concurrent import run_on_executor

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_channel import IpcChannelException
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError


SOURCE_ENDPOINTS_CONFIG_NAME = 'source_endpoints'
DEST_ENDPOINT_CONFIG_NAME = 'destination_endpoint'
QUEUE_LENGTH_CONFIG_NAME = 'queue_length'

DEFAULT_SOURCE_ENDPOINTS = "tcp://127.0.0.1:5010"
DEFAULT_DEST_ENDPOINT = "tcp://127.0.0.1:5020"
DEFAULT_QUEUE_LENGTH = 10


class LiveViewProxyAdapter(ApiAdapter):
    """
    Live View Proxy Adapter Class

    Implements the live view proxy adapter for odin control
    """

    # Thread executor used for background tasks
    executor = futures.ThreadPoolExecutor(max_workers=1)

    def __init__(self, **kwargs):

        logging.debug("Live View Proxy Adapter init called")
        super(LiveViewProxyAdapter, self).__init__(**kwargs)

        self.dest_endpoint = self.options.get(DEST_ENDPOINT_CONFIG_NAME,
                                              DEFAULT_DEST_ENDPOINT).strip()

        try:
            logging.debug("Connecting publish socket to endpoint: %s", self.dest_endpoint)
            self.publish_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_PUB,
                                                     self.dest_endpoint)
            self.publish_channel.bind()
        except IpcChannelException as channel_err:
            logging.error("Connection Failed. Error given: %s", channel_err.message)
        self.max_queue = self.options.get(QUEUE_LENGTH_CONFIG_NAME, DEFAULT_QUEUE_LENGTH)

        if SOURCE_ENDPOINTS_CONFIG_NAME in self.options:
            self.source_endpoints = []
            for target_str in self.options[SOURCE_ENDPOINTS_CONFIG_NAME].split(','):
                try:
                    (target, url) = target_str.strip().split("=")
                    self.source_endpoints.append(LiveViewProxyNode(target, url, self.add_to_queue))
                except ValueError:
                    logging.debug("Error parsing target list: %s", target_str)
        else:
            self.source_endpoints = [LiveViewProxyNode(
                "node_1",
                DEFAULT_SOURCE_ENDPOINTS,
                self.add_to_queue)]

        tree = {
            "target_endpoint": (self.get_target_endpoint, None),
            'last_sent_frame': (self.get_last_frame, None),
            'dropped_frames': (self.get_dropped_frames, None),
            'reset': (None, self.set_reset),
            "nodes": {}
        }
        for sub in self.source_endpoints:
            tree["nodes"][sub.get_name()] = sub.param_tree

        self.param_tree = ParameterTree(tree)

        self.queue = PriorityQueue(self.max_queue)

        self.last_sent_frame = (0, 0)
        self.dropped_frame_count = 0

        self.get_frame_from_queue(0)

    @run_on_executor
    def get_frame_from_queue(self, task_interval):
        """
        loop to pop frames off the queue and send them to the destination ZMQ socket
        """
        try:
            frame = self.queue.get_nowait()
            logging.debug("Got frame %d:%d from queue", frame.acq_id, frame.get_number())
            self.last_sent_frame = (frame.get_acq(), frame.get_number())
            self.publish_channel.send_multipart([frame.get_header(), frame.get_data()])
        except QueueEmptyException:
            # queue is empty but thats fine, no need to report
            # or there'd be far too much output
            pass
        finally:
            time.sleep(task_interval)
            IOLoop.instance().add_callback(self.get_frame_from_queue, task_interval)

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        response = self.param_tree.get(path)
        content_type = 'application/json'
        status = 200
        return ApiAdapterResponse(response, content_type, status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        try:
            data = json_decode(request.body)
            self.param_tree.set(path, data)
            response = self.param_tree.get(path)
            status_code = 200
        except ParameterTreeError as set_err:
            response = {'error': str(set_err)}
            status_code = 400
        return ApiAdapterResponse(response, status_code=status_code)

    def cleanup(self):
        pass

    def add_to_queue(self, frame, source):
        """
        Add the frame to the priority queue, so long as it's "new enough" (the frame number is
        not lower than the last sent frame)
        If the queue is full, the next frame should be removed and this frame added instead.
        """
        logging.debug("Add to queue called from %s", source.get_name())
        logging.debug("Adding frame number: %d", frame.get_number())
        if (frame.get_acq(), frame.get_number()) < self.last_sent_frame:
            logging.debug("Frame too old, disgarding")
            source.dropped_frame()
            return
        try:
            self.queue.put_nowait(frame)
        except QueueFullException:
            logging.debug("Queue Full, discarding frame")
            self.queue.get_nowait()
            self.queue.put_nowait(frame)
            self.dropped_frame_count += 1

    def get_last_frame(self):
        """
        return the frame number of the last sent frame
        """
        return self.last_sent_frame

    def get_target_endpoint(self):
        """
        return the endpoint address of the target endpoint
        """
        return self.dest_endpoint

    def get_dropped_frames(self):
        """
        return the number of frames dropped due to the queue being full
        """
        return self.dropped_frame_count

    def set_reset(self, data):
        """
        reset the statistics for a new aquisition, setting dropped frames and sent frame
        counters back to 0
        """
        # TODO: find way to stop queue accepting new frames while resetting
        while not self.queue.empty():
            pass  # wait for queue to empty before resetting
        self.last_sent_frame = (0, 0)
        self.dropped_frame_count = 0
        for node in self.source_endpoints:
            node.set_reset()


class LiveViewProxyNode(object):
    """
    Live View Proxy. Connect to a ZMQ socket from an Odin Data Live View Plugin, saving any frames
    that arrive in a queue and passing them on to the central proxy controller when needed.
    """
    def __init__(self, name, endpoint, callback):
        logging.debug("Live View Proxy Initialising: %s", endpoint)
        self.name = name
        self.endpoint = endpoint
        self.callback = callback
        self.dropped_frame_count = 0
        self.received_frame_count = 0
        self.last_frame = 0
        self.current_acq = 0

        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        self.channel.register_callback(self.local_callback)

        self.param_tree = ParameterTree({
            'endpoint': (self.get_endpoint, None),
            'dropped_frames': (self.get_dropped_frames, None),
            'received_frames': (self.get_received_frames, None),
            'last_frame': (self.get_last_frame, None),
            'current_acquisition': (self.get_current_acq, None)
        })

        logging.debug("Proxy Connected to Socket: %s", endpoint)

    def local_callback(self, msg):
        """
        Create an instance of the Frame class using the data from the socket.
        Then, pass the frame on to the adapter to add to the Priority Queue.
        """
        tmp_frame = Frame(msg)
        self.received_frame_count += 1
        if tmp_frame.get_number() < self.last_frame:
            logging.debug(
                "New Frame %d has lower number than last frame %d. Has a new acquisition started?",
                tmp_frame.get_number(), self.last_frame
            )
            self.current_acq += 1

        tmp_frame.set_acq(self.current_acq)
        self.last_frame = tmp_frame.get_number()
        self.callback(tmp_frame, self)

    def get_endpoint(self):
        """
        return the endpoint address of the source socket
        """
        return self.endpoint

    def get_dropped_frames(self):
        """
        return the number of frames dropped due to them being older than the last sent frame
        """
        return self.dropped_frame_count

    def get_received_frames(self):
        """
        return the number of frames received from the source socket
        """
        return self.received_frame_count

    def get_name(self):
        """
        return the node name
        """
        return self.name

    def get_last_frame(self):
        """
        return the number of the last frame received by this node
        """
        return self.last_frame

    def get_current_acq(self):
        """
        return current acquisition number
        """
        return self.current_acq

    def dropped_frame(self):
        """
        increase the count of dropped frames due to frame age
        """
        self.dropped_frame_count += 1

    def set_reset(self):
        """
        reset frame counts for dropped and received frames for starting
        a new aqquisition
        """
        self.dropped_frame_count = 0
        self.received_frame_count = 0
        self.last_frame = 0
        self.current_acq = 0


class Frame(object):
    """
    Class to hold the frame data. Also has a method that allows it to be sorted via the frame number
    """
    def __init__(self, msg):
        self.header = json_decode(msg[0])
        self.data = msg[1]
        self.num = self.header["frame_num"]
        self.acq_id = 0

    def __lt__(self, other):  # used for sorting
        if self.acq_id == other.acq_id:  # if from same acquisition, use the frame number
            return self.num < other.num
        return self.acq_id < other.acq_id

    def get_header(self):
        """
        Return the frame header as a json encoded string
        """
        return json_encode(self.header)

    def get_number(self):
        """
        Return the frame number
        """
        return self.num

    def get_data(self):
        """
        Return the data of the frame
        """
        return self.data
    
    def get_acq(self):
        """
        Return the acquisition ID for the frame
        """
        return self.acq_id

    def set_acq(self, acq):
        """
        Sets the acquisition ID for the frame
        """
        self.acq_id = acq
