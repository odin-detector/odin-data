"""
Created on 6th September 2017

:author: Alan Greer
"""
import json
import logging
from odin_data.ipc_tornado_client import IpcTornadoClient
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from tornado import escape
from tornado.ioloop import IOLoop


class OdinDataAdapter(ApiAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    ERROR_NO_RESPONSE = "No response from client, check it is running"
    ERROR_FAILED_TO_SEND = "Unable to successfully send request to client"
    ERROR_FAILED_GET = "Unable to successfully complete the GET request"
    ERROR_PUT_MISMATCH = "The size of parameter array does not match the number of clients"

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        super(OdinDataAdapter, self).__init__(**kwargs)

        self._endpoint_arg = None
        self._endpoints = []
        self._clients = []
        self._update_interval = None

        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]
        self._kwargs['module'] = self.name

        try:
            self._endpoint_arg = self.options.get('endpoints')
        except:
            raise RuntimeError("No endpoints specified for the frameProcessor client(s)")

        for arg in self._endpoint_arg.split(','):
            arg = arg.strip()
            logging.debug("Endpoint: %s", arg)
            ep = {'ip_address': arg.split(':')[0],
                  'port': int(arg.split(':')[1])}
            self._endpoints.append(ep)
        self._kwargs['endpoints'] = self._endpoints

        for ep in self._endpoints:
            self._clients.append(IpcTornadoClient(ep['ip_address'], ep['port']))

        self._kwargs['count'] = len(self._clients)
        # Allocate the status list
        self._status = [None] * len(self._clients)

        # Setup the time between client update requests
        self._update_interval = float(self.options.get('update_interval', 0.5))
        self._kwargs['update_interval'] = self._update_interval
        # Start up the status loop
        self.update_loop()

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
        logging.debug("GET path: %s", path)
        logging.debug("GET request: %s", request)

        # Check if the adapter type is being requested
        request_command = path.strip('/')
        if not request_command:
            key_list = self._kwargs.keys()
            for client in self._clients:
                for key in client.parameters:
                    if key not in key_list:
                        key_list.append(key)
            response[request_command] = key_list
        elif request_command in self._kwargs:
            logging.debug("Adapter request for ini argument: %s", request_command)
            response[request_command] = self._kwargs[request_command]
        else:
            try:
                request_items = request_command.split('/')
                logging.debug(request_items)
                # Now we need to traverse the parameters looking for the request

                response_items = []
                for client in self._clients:
                    paramset = client.parameters
                    logging.debug("Client paramset: %s", paramset)
                    try:
                        item_dict = paramset
                        for item in request_items:
                            item_dict = item_dict[item]
                    except KeyError, ex:
                        logging.debug("Invalid parameter request in HTTP GET: %s", request_command)
                        item_dict = None
                    response_items.append(item_dict)

                logging.debug(response_items)
                response['value'] = response_items
            except:
                logging.debug(OdinDataAdapter.ERROR_FAILED_GET)
                status_code = 503
                response['error'] = OdinDataAdapter.ERROR_FAILED_GET

        logging.debug("Full response from FP: %s", response)

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
        logging.debug("PUT request.body: %s", str(escape.url_unescape(request.body)))

        request_command = path.strip('/')

        # Request should start with config/
        if request_command.startswith("config/"):
            request_command = request_command.replace("config/", "", 1)  # Take the rest of the URI
            logging.debug("Configure URI: %s", request_command)
            try:
                parameters = json.loads(str(escape.url_unescape(request.body)))
            except ValueError:
                # If the body could not be parsed into an object it may be a simple string
                parameters = str(escape.url_unescape(request.body))

            # Check if the parameters object is a list
            if isinstance(parameters, list):
                logging.debug("List of parameters provided: %s", parameters)
                # Check the length of the list matches the number of clients
                if len(parameters) != len(self._clients):
                    status_code = 503
                    response['error'] = OdinDataAdapter.ERROR_PUT_MISMATCH
                else:
                    # Loop over the clients and parameters, sending each one
                    for client, param_set in zip(self._clients, parameters):
                        if param_set:
                            try:
                                client.send_configuration(param_set, request_command)
                            except Exception as err:
                                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                                logging.error("Error: %s", err)
                                status_code = 503
                                response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}

            else:
                logging.debug("Single parameter set provided: %s", parameters)
                for client in self._clients:
                    try:
                        client.send_configuration(parameters, request_command)
                    except Exception as err:
                        logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                        logging.error("Error: %s", err)
                        status_code = 503
                        response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def delete(self, path, request):  # pylint: disable=W0613
        """
        Implementation of the HTTP DELETE verb for OdinDataAdapter

        :param path: URI path of the DELETE request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        response = {'response': '{}: DELETE on path {}'.format(self.name, path)}
        status_code = 501

        logging.debug(response)

        return ApiAdapterResponse(response, status_code=status_code)

    def update_loop(self):
        """Handle background update loop tasks.
        This method handles background update tasks executed periodically in the tornado
        IOLoop instance. This includes requesting the status from the underlying application
        and preparing the JSON encoded reply in a format that can be easily parsed.
        """
        logging.debug("Updating status from client...")

        # Handle background tasks
        # Loop over all connected clients and obtain the status
        for client in self._clients:
            # First check for stale status within a client
            client.check_for_stale_status(self._update_interval * 10)
            # Request a configuration update
            client.send_request('request_configuration')
            # Now request a status update
            client.send_request('status')

        # Schedule the update loop to run in the IOLoop instance again after appropriate interval
        IOLoop.instance().call_later(self._update_interval, self.update_loop)

