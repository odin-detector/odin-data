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
                

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        pass

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        pass

    def cleanup(self):
        pass


class LiveViewProxy(object):

    def __init__(self, source_endpoints, target_endpoint):
        pass

