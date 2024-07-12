"""
Created on 6th September 2017

:author: Alan Greer
"""

import logging

from odin.adapters.adapter import (
    ApiAdapter,
    ApiAdapterResponse,
    request_types,
    response_types,
    wants_metadata,
)
from odin.adapters.parameter_tree import (
    ParameterTreeError,
)
from tornado import escape
from tornado.escape import json_decode

from odin_data.control.frame_processor_controller import FrameProcessorController

FP_ADAPTER_DEFAULT_KEY = "od_fps"
FP_ADAPTER_KEY = "fp_adapter_key"


class FrameProcessorAdapter(ApiAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        super(FrameProcessorAdapter, self).__init__(**kwargs)

        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]

        self._fp_adapter_name = FP_ADAPTER_DEFAULT_KEY
        if FP_ADAPTER_KEY in kwargs:
            self._fp_adapter_name = kwargs[FP_ADAPTER_KEY]

        self._fp_adapter = None

        # Create the Frame Processor Controller object
        self._controller = FrameProcessorController(self.name)

    def initialize(self, adapters):
        """Initialize the adapter after it has been loaded.
        Find and record the FR adapter for later error checks
        """
        if self._fp_adapter_name in adapters:
            self._fp_adapter = adapters[self._fp_adapter_name]
            logging.info(
                "FP adapter initiated connection to OdinData raw adapter: {}".format(
                    self._fp_adapter_name
                )
            )
        else:
            logging.error(
                "FP adapter could not connect to the OdinData raw adapter: {}".format(
                    self._fp_adapter_name
                )
            )

        self._controller.initialize(None, self._fp_adapter)

    @request_types("application/json", "application/vnd.odin-native")
    @response_types("application/json", default="application/json")
    def get(self, path, request):
        """
        Implementation of the HTTP GET verb for OdinDataAdapter

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        response = {}
        status_code = 200
        content_type = "application/json"

        logging.debug("Get Request: {}".format(path))
        try:
            response = self._fp_adapter._controller.get(path, wants_metadata(request))
            logging.debug("Get Response: {}".format(response))
        except ParameterTreeError as param_error:
            response = {
                "response": "OdinDatatAdapter GET error: {}".format(param_error)
            }
            status_code = 400

        return ApiAdapterResponse(
            response, content_type=content_type, status_code=status_code
        )

    @request_types("application/json", "application/vnd.odin-native")
    @response_types("application/json", default="application/json")
    def put(self, path, request):  # pylint: disable=W0613
        """
        Implementation of the HTTP PUT verb for OdinDataAdapter

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}
        logging.debug("PUT path: %s", path)
        logging.debug("PUT request: %s", escape.url_unescape(request.body))

        try:
            self._fp_adapter._controller.put(path, json_decode(request.body))
        except Exception:
            return super(FrameProcessorAdapter, self).put(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    def cleanup(self):
        self._controller.shutdown()
