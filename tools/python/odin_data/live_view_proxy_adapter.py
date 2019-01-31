"""ODIN data live view proxy adapter.

This module implements an odin-control adapter capable of subscring to
multiple Odin Data Live View plugins and combining all streams of frames
into a single ZMQ stream.

Created on 28th January 2019

:author: Adam Neaves, STFC Application Engineering Group
"""
import logging

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_channel import IpcChannelException
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError

SOURCE_ENDPOINTS_CONFIG_NAME = 'source_endpoints'
DEST_ENDPOINT_CONFIG_NAME = 'destination_endpoint'


class LiveViewProxyAdapter(ApiAdapter):
    """
    Live View Proxy Adapter Class

    Implements the live view proxy adapter for odin control
    """

    def __init__(self, **kwargs):

        logging.debug("Live View Proxy Adapter init called")
        super(LiveViewProxyAdapter, self).__init__(**kwargs)

        if self.options.get(SOURCE_ENDPOINTS_CONFIG_NAME, False):
            self.source_endpoints = []
            for endpoint in self.options.get(SOURCE_ENDPOINTS_CONFIG_NAME, "").split(','):
                self.source_endpoints.append(endpoint.strip())

        if self.options.get(DEST_ENDPOINT_CONFIG_NAME, False):
            self.dest_endpoint = self.options.get(DEST_ENDPOINT_CONFIG_NAME, "").strip()

        self.param_tree = ParameterTree({
            "source_endpoints": (self.source_endpoints, None),
            "target_endpoint": (self.dest_endpoint, None)
        }) 

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


class LiveViewProxy(object):
    """

    """
    def __init__(self, source_endpoints, target_endpoint):
        logging.debug("Live View Proxy Initialising")

        self.sub_channels = []
        for endpoint in source_endpoints:
            tmp_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
            tmp_channel.subscribe()
            tmp_channel.connect()
            tmp_channel.register_callback(self.pub_if_newer)
            self.sub_channels.append(tmp_channel)

        self.pub_channel = IpcTornadoChannel(
            IpcTornadoChannel.CHANNEL_TYPE_PUB,
            endpoint=target_endpoint)

    def pub_if_newer(self, msg):
        """

        """
        # TODO: compare when frame was sent, if older than most recent shown one discard it
        self.pub_channel.send(msg)
        