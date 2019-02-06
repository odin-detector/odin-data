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

from tornado.escape import json_decode

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_channel import IpcChannelException
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError


SOURCE_ENDPOINTS_CONFIG_NAME = 'source_endpoints'
DEST_ENDPOINT_CONFIG_NAME = 'destination_endpoint'
QUEUE_LENGTH_CONFIG_NAME = 'queue_length'

DEFAULT_SOURCE_ENDPOINTS = ["tcp://127.0.0.1:5010"]
DEFAULT_DEST_ENDPOINT = "tcp://127.0.0.1:5020"
DEFAULT_QUEUE_LENGTH = 5


class LiveViewProxyAdapter(ApiAdapter):
    """
    Live View Proxy Adapter Class

    Implements the live view proxy adapter for odin control
    """

    def __init__(self, **kwargs):

        logging.debug("Live View Proxy Adapter init called")
        super(LiveViewProxyAdapter, self).__init__(**kwargs)

        self.dest_endpoint = self.options.get(DEST_ENDPOINT_CONFIG_NAME,
                                              DEFAULT_DEST_ENDPOINT).strip()

        logging.debug("Connecting publish socket to endpoint: %s", self.dest_endpoint)
        self.publish_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_PUB,
                                                 self.dest_endpoint)
        self.publish_channel.bind()

        self.max_queue = self.options.get(QUEUE_LENGTH_CONFIG_NAME, DEFAULT_QUEUE_LENGTH)

        if self.options.get(SOURCE_ENDPOINTS_CONFIG_NAME, False):
            self.source_endpoints = []
            for target_str in self.options.get(SOURCE_ENDPOINTS_CONFIG_NAME).split(','):
                try:
                    (target, url) = target_str.strip().split("=")
                    self.source_endpoints.append(LiveViewProxyNode(target, url, self.add_to_queue))
                except ValueError:
                    logging.debug("Error parsing target list: %s", target_str)
        else:
            self.source_endpoints = LiveViewProxyNode(
                "node_1",
                DEFAULT_SOURCE_ENDPOINTS,
                self.add_to_queue)

        tree = {
            "target_endpoint": (self.get_target_endpoint, None),
            'last_sent_frame': (self.get_last_frame, None),
            'dropped_frames': (self.get_dropped_frames, None),
            "nodes": {}
        }
        for sub in self.source_endpoints:
            tree["nodes"][sub.get_name()] = sub.param_tree

        self.param_tree = ParameterTree(tree)

        self.queue = PriorityQueue(self.max_queue)

        self.last_sent_frame = 0
        self.dropped_frame_count = 0

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        response = self.param_tree.get(path)
        content_type = 'application/json'
        status = 200
        return ApiAdapterResponse(response, content_type, status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        response = "PUT method not implemented by {}".format(self.name)
        return ApiAdapterResponse(response, status_code=400)

    def cleanup(self):
        pass

    def get_target_endpoint(self):
        return self.dest_endpoint
    
    def get_dropped_frames(self):
        return self.dropped_frame_count

    def add_to_queue(self, frame, source):
        logging.debug("Add to queue called from %s", source.get_name())
        logging.debug("Adding frame number: %d", frame.get_number())
        if(frame.get_number() < self.last_sent_frame):
            logging.debug("Frame too old, disgarding")
            source.dropped_frame_count += 1
            return
        try:
            self.queue.put_nowait(frame)
        except QueueFullException:
            logging.debug("Queue Full, discarding frame")
            self.queue.get_nowait()
            self.queue.put_nowait(frame)
            self.dropped_frame_count += 1

    def get_last_frame(self):
        return self.last_sent_frame


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

        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        self.channel.register_callback(self.local_callback)

        self.param_tree = ParameterTree({
            'endpoint': (self.get_endpoint, None),
            'dropped_frames': (self.get_dropped_frames, None),
        })

        logging.debug("Proxy Connected to Socket: %s", endpoint)

    def local_callback(self, msg):
        """
        Creates an instance of the Frame class using the data from the socket.
        Then, passed the frame on to the adapter to add to the Priority Queue.
        If the priority queue is full, it will throw an exception and this frame will be discarded
        """
        tmp_frame = Frame(msg)
        self.callback(tmp_frame, self)

    def get_endpoint(self):
        return self.endpoint

    def get_dropped_frames(self):
        return self.dropped_frame_count

    def get_name(self):
        return self.name


class Frame(object):

    def __init__(self, msg):
        self.header = json_decode(msg[0])
        self.data = msg[1]
        self.num = self.header["frame_num"]

    def __lt__(self, other):  # used for sorting
        return self.num < other.num

    def get_header(self):
        return self.header

    def get_number(self):
        return self.num
