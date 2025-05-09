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

from odin_data.control.odin_data_controller import OdinDataController


class OdinDataAdapter(ApiAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    _controller_cls = OdinDataController

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        super(OdinDataAdapter, self).__init__(**kwargs)

        self._endpoint_arg = None
        self._update_interval = None

        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]

        try:
            self._endpoint_arg = self.options.get("endpoints")
        except Exception:
            raise RuntimeError(
                "No endpoints specified for the frameProcessor client(s)"
            )

        # Setup the time between client update requests
        self._update_interval = float(self.options.get("update_interval", 0.5))

        # Create the Frame Processor Controller object
        self._controller = self._controller_cls(
            self.name, self._endpoint_arg, self._update_interval
        )

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

        try:
            response = self._controller.get(path, wants_metadata(request))
            logging.debug("GET {}".format(path))
            logging.debug("GET response: {}".format(response))
        except ParameterTreeError as param_error:
            response = {
                "response": f"{type(self).__name__} GET error: {param_error}"
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
            self._controller.put(path, json_decode(request.body))
        except ParameterTreeError as param_error:
            response = {
                "response": f"{type(self).__name__} PUT error: {param_error}"
            }
            status_code = 400

        return ApiAdapterResponse(response, status_code=status_code)

    def cleanup(self):
        self._controller.shutdown()
