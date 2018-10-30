"""
Created on 6th September 2017

:author: Alan Greer
"""
import json
import logging
import os
from odin_data.ipc_tornado_client import IpcTornadoClient
from odin_data.util import remove_prefix, remove_suffix
from odin_data.odin_data_adapter import OdinDataAdapter
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from tornado import escape
from tornado.ioloop import IOLoop


def bool_from_string(value):
    bool_value = False
    if value.lower() == 'true' or value.lower() == '1':
        bool_value = True
    return bool_value


class FrameProcessorAdapter(OdinDataAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    VERSION_CHECK_CONFIG_ITEMS = ['plugin']

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        logging.debug("FPA init called")
        super(FrameProcessorAdapter, self).__init__(**kwargs)

        self._param = {
            'config/hdf/file/path': '',
            'config/hdf/file/name': '',
            'config/hdf/file/extension': 'h5',
            'config/hdf/frames': 0
        }
        self._command = 'config/hdf/write'
        self.setup_rank()

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def get(self, path, request):

        """
        Implementation of the HTTP GET verb for OdinDataAdapter

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}

        # First check if we are interested in the config items
        #
        # Store these parameters locally:
        # config/hdf/file/path
        # config/hdf/file/name
        # config/hdf/file/extension
        #
        # When this arrives write all params into a single IPC message
        # config/hdf/write
        if path in self._param:
            response['value'] = self._param[path]
        else:
            return super(FrameProcessorAdapter, self).get(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
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
        logging.debug("PUT request: %s", request)

        # First check if we are interested in the config items
        #
        # Store these parameters locally:
        # config/hdf/file/path
        # config/hdf/file/name
        # config/hdf/file/extension
        #
        # When this arrives write all params into a single IPC message
        # config/hdf/write
        try:
            self.clear_error()
            if path in self._param:
                logging.debug("Setting {} to {}".format(path, str(escape.url_unescape(request.body)).replace('"', '')))
                if path == 'config/hdf/frames':
                    self._param[path] = int(str(escape.url_unescape(request.body)).replace('"', ''))
                else:
                    self._param[path] = str(escape.url_unescape(request.body)).replace('"', '')
                # Merge with the configuration store

            elif path == self._command:
                write = bool_from_string(str(escape.url_unescape(request.body)))
                config = {'hdf': {'write': write}}
                logging.debug("Setting {} to {}".format(path, config))
                if write:
                    # Before attempting to write files, make some simple error checks
                    # Check the file path is valid
                    if not os.path.isdir(str(self._param['config/hdf/file/path'])):
                        raise RuntimeError("Invalid path specified [{}]".format(str(self._param['config/hdf/file/path'])))
                    # Check the filename exists
                    if str(self._param['config/hdf/file/name']) == '':
                        raise RuntimeError("File name must not be empty")

                    # First setup the rank for the frameProcessor applications
                    self.setup_rank()
                    rank = 0
                    for client in self._clients:
                        # Send the configuration required to setup the acquisition
                        # The file path is the same for all clients
                        parameters = {
                            'hdf': {
                                'frames': self._param['config/hdf/frames']
                            }
                        }
                        # Send the number of frames first
                        client.send_configuration(parameters)
                        parameters = {
                            'hdf': {
                                'file': {
                                    'path': str(self._param['config/hdf/file/path']),
                                    'name': str(self._param['config/hdf/file/name']),
                                    'extension': str(self._param['config/hdf/file/extension'])
                                    }
                            }
                        }
                        client.send_configuration(parameters)
                        rank += 1
                for client in self._clients:
                    # Send the configuration required to start the acquisition
                    client.send_configuration(config)

            else:
                response = super(FrameProcessorAdapter, self).put(path, request)
                # Check that the base call has been successful
                if response.status_code == 200:
                    if path.startswith("config/"):
                        path = remove_prefix(path, "config/")
                    # Retrieve the top level command from the path and parameters
                    command, parameters = self.uri_params_to_dictionary(path, request.body)
                    # If the top level command is in the version check list then request a version update
                    if command in self.VERSION_CHECK_CONFIG_ITEMS:
                        self.request_version()
                return response

        except Exception as ex:
            logging.error("Error: %s", ex)
            self.set_error(str(ex))
            status_code = 503
            response = {'error': str(ex)}

        return ApiAdapterResponse(response, status_code=status_code)

    def setup_rank(self):
        # Attempt initialisation of the connected clients
        processes = len(self._clients)
        rank = 0
        for client in self._clients:
            try:
                # Setup the number of processes and the rank for each client
                parameters = {
                    'hdf': {
                        'process': {
                            'number': processes,
                            'rank': rank
                        }
                    }
                }
                client.send_configuration(parameters)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)
            rank += 1
