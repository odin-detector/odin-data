"""ODIN data live view proxy adapter.

This module implements an odin-control adapter capable of subscribing to
multiple Odin Data Live View plugins and combining all streams of frames
into a single ZMQ stream.

Created on 28th January 2019

:author: Ashley Neaves, STFC Application Engineering Group
"""
import logging
from queue import PriorityQueue
from queue import Full as QueueFullException
from queue import Empty as QueueEmptyException

from tornado.escape import json_decode, json_encode
from tornado.ioloop import IOLoop

from zmq.error import ZMQError

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError


SOURCE_ENDPOINTS_CONFIG_NAME = 'source_endpoints'
DEST_ENDPOINT_CONFIG_NAME = 'destination_endpoint'
QUEUE_LENGTH_CONFIG_NAME = 'queue_length'
DROP_WARN_CONFIG_NAME = 'dropped_frame_warning_cutoff'

DEFAULT_SOURCE_ENDPOINT = "tcp://127.0.0.1:5010"
DEFAULT_DEST_ENDPOINT = "tcp://127.0.0.1:5020"
DEFAULT_QUEUE_LENGTH = 10
DEFAULT_DROP_WARN_PERCENT = 0.5


class LiveViewProxyAdapter(ApiAdapter):
    """
    Live View Proxy Adapter Class

    Implements the live view proxy adapter for odin control
    """

    def __init__(self, **kwargs):
        """
        Initialise the Adapter, using the provided configuration.
        Create the node classes for the subscriptions to multiple ZMQ sockets.
        Also create the publish socket to push frames onto.
        """
        logging.debug("Live View Proxy Adapter init called")
        super(LiveViewProxyAdapter, self).__init__(**kwargs)

        self.dest_endpoint = self.options.get(DEST_ENDPOINT_CONFIG_NAME,
                                              DEFAULT_DEST_ENDPOINT).strip()

        self.drop_warn_percent = float(self.options.get(DROP_WARN_CONFIG_NAME,
                                                        DEFAULT_DROP_WARN_PERCENT))

        try:
            logging.debug("Connecting publish socket to endpoint: %s", self.dest_endpoint)
            self.publish_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_PUB,
                                                     self.dest_endpoint)
            self.publish_channel.bind()
        except ZMQError as channel_err:
            # ZMQError raised here if the socket addr is already in use.
            logging.error("Connection Failed. Error given: %s", channel_err.message)
        self.max_queue = self.options.get(QUEUE_LENGTH_CONFIG_NAME, DEFAULT_QUEUE_LENGTH)

        if SOURCE_ENDPOINTS_CONFIG_NAME in self.options:
            self.source_endpoints = []
            for target_str in self.options[SOURCE_ENDPOINTS_CONFIG_NAME].split(','):
                try:
                    # config provides the nodes as "node_name=socket_URI" pairs. Split those strings
                    (target, url) = target_str.split("=")
                    self.source_endpoints.append(LiveViewProxyNode(
                        target.strip(),
                        url.strip(),
                        self.drop_warn_percent,
                        self.add_to_queue))
                except (ValueError, ZMQError):
                    logging.debug("Error parsing target list: %s", target_str)
        else:
            self.source_endpoints = [LiveViewProxyNode(
                "node_1",
                DEFAULT_SOURCE_ENDPOINT,
                self.drop_warn_percent,
                self.add_to_queue)]

        tree = {
            "target_endpoint": (lambda: self.dest_endpoint, None),
            'last_sent_frame': (lambda: self.last_sent_frame, None),
            'dropped_frames': (lambda: self.dropped_frame_count, None),
            'reset': (None, self.set_reset),
            "nodes": {}
        }
        for sub in self.source_endpoints:
            tree["nodes"][sub.name] = sub.param_tree

        self.param_tree = ParameterTree(tree)

        self.queue = PriorityQueue(self.max_queue)

        self.last_sent_frame = (0, 0)
        self.dropped_frame_count = 0

        self.get_frame_from_queue()

    def cleanup(self):
        """
        Ensure that, on shutdown, all ZMQ sockets are closed
        so that they do not linger and potentially cause issues in the future
        """
        self.publish_channel.close()
        for node in self.source_endpoints:
            node.cleanup()

    def get_frame_from_queue(self):
        """
        Loop to pop frames off the queue and send them to the destination ZMQ socket.
        """
        frame = None
        try:
            frame = self.queue.get_nowait()
            self.last_sent_frame = (frame.acq_id, frame.num)
            self.publish_channel.send_multipart([frame.get_header(), frame.data])
        except QueueEmptyException:
            # queue is empty but thats fine, no need to report
            # or there'd be far too much output
            pass
        finally:
            IOLoop.instance().call_later(0, self.get_frame_from_queue)
        return frame  # returned for testing

    @response_types('application/json', default='application/json')
    def get(self, path, request):
        """
        HTTP Get Request Handler. Return the requested data from the parameter tree
        """
        response = self.param_tree.get(path)
        content_type = 'application/json'
        status = 200
        return ApiAdapterResponse(response, content_type, status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        """
        HTTP Put Request Handler. Return the requested data after changes were made.
        """
        try:
            data = json_decode(request.body)
            self.param_tree.set(path, data)
            response = self.param_tree.get(path)
            status_code = 200
        except ParameterTreeError as set_err:
            response = {'error': str(set_err)}
            status_code = 400
        return ApiAdapterResponse(response, status_code=status_code)

    def add_to_queue(self, frame, source):
        """
        Add the frame to the priority queue, so long as it's "new enough" (the frame number is
        not lower than the last sent frame)
        If the queue is full, the next frame should be removed and this frame added instead.
        """
        if (frame.acq_id, frame.num) < self.last_sent_frame:
            source.dropped_frame()
            return
        try:
            self.queue.put_nowait(frame)
        except QueueFullException:
            logging.debug("Queue Full, discarding frame")
            self.queue.get_nowait()
            self.queue.put_nowait(frame)
            self.dropped_frame_count += 1

    def set_reset(self, data):
        """
        Reset the statistics for a new aquisition, setting dropped frames and sent frame
        counters back to 0
        """
        # we ignore the "data" parameter, as it doesn't matter what was actually PUT to
        # the method to reset.
        self.last_sent_frame = (0, 0)
        self.dropped_frame_count = 0
        for node in self.source_endpoints:
            node.set_reset()


class LiveViewProxyNode(object):
    """
    Live View Proxy. Connect to a ZMQ socket from an Odin Data Live View Plugin, saving any frames
    that arrive in a queue and passing them on to the central proxy controller when needed.
    """
    def __init__(self, name, endpoint, drop_warn_cutoff, callback):
        """
        Initialise a Node for the Adapter. The node should subscribe to
        a ZMQ socket and pass any frames received at that socket to the main
        adapter.
        """
        logging.debug("Live View Proxy Node Initialising: %s", endpoint)
        self.name = name
        self.endpoint = endpoint
        self.callback = callback
        self.dropped_frame_count = 0
        self.received_frame_count = 0
        self.last_frame = 0
        self.current_acq = 0
        self.drop_warn_cutoff = drop_warn_cutoff
        self.drop_unwarn_cutoff = drop_warn_cutoff * 0.75
        self.has_warned = False
        # subscribe to the given socket address.
        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        # callback is called whenever data 'arrives' at the socket. This is driven by the IOLoop
        self.channel.register_callback(self.local_callback)

        self.param_tree = ParameterTree({
            'endpoint': (lambda: self.endpoint, None),
            'dropped_frames': (lambda: self.dropped_frame_count, None),
            'received_frames': (lambda: self.received_frame_count, None),
            'last_frame': (lambda: self.last_frame, None),
            'current_acquisition': (lambda: self.current_acq, None),
        })

        logging.debug("Proxy Connected to Socket: %s", endpoint)

    def cleanup(self):
        """
        Close the Subscriber socket for this node.
        """
        self.channel.close()

    def local_callback(self, msg):
        """
        Create an instance of the Frame class using the data from the socket.
        Then, pass the frame on to the adapter to add to the Priority Queue.
        """
        frame = Frame(msg)
        self.received_frame_count += 1
        if frame.num < self.last_frame:
            # Frames are assumed to be in order for each socket. If a frame suddenly has
            # a lower frame number, it can be assumed that a new acquisition has begun.
            logging.debug(
                "Frame number has reset, new acquisition started on Node %s",
                self.name
            )
            self.current_acq += 1
            self.has_warned = False

        frame.set_acq(self.current_acq)
        self.last_frame = frame.num
        self.callback(frame, self)

    def dropped_frame(self):
        """
        Increase the count of dropped frames due to frame age
        """
        self.dropped_frame_count += 1
        current_dropped_percent = float(self.dropped_frame_count) / float(self.received_frame_count)
        if current_dropped_percent > self.drop_warn_cutoff and not self.has_warned:
            # If the number of dropped frames reaches a certain percentage threshold of the total
            # received, warn the user, as it likely means one node is running slow and is unlikely
            # to display any frames.
            logging.warning(
                "Node %s has dropped %d%% of frames",
                self.name,
                current_dropped_percent*100)
            self.has_warned = True
        elif current_dropped_percent < self.drop_unwarn_cutoff:
            self.has_warned = False

    def set_reset(self):
        """
        Reset frame counts for dropped and received frames for starting
        a new aqquisition
        """
        self.dropped_frame_count = 0
        self.received_frame_count = 0
        self.last_frame = 0
        self.current_acq = 0
        self.has_warned = False


class Frame(object):
    """
    Container class for frame data that allows frames to be sorted
    by frame and acquisition number in a Priority Queue.
    """
    def __init__(self, msg):
        """
        Decode the message header and get the frame number from it.
        Also save a reference to the frame data.
        """
        self.header = json_decode(msg[0])
        self.data = msg[1]
        self.num = self.header["frame_num"]
        self.acq_id = 0

    def __lt__(self, other):
        """
        Return if the current frame is 'less than' the other frame,
        in regards to the frame and acqusitition number.
        """
        if self.acq_id == other.acq_id:  # if from same acquisition, use the frame number
            return self.num < other.num
        return self.acq_id < other.acq_id

    def get_header(self):
        """
        Return the frame header as a json encoded string
        """
        return json_encode(self.header)

    def set_acq(self, acq):
        """
        Sets the acquisition ID for the frame
        """
        self.acq_id = acq
