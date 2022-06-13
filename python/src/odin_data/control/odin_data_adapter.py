"""
Created on 6th September 2017

:author: Alan Greer
"""
import json
import logging

from odin.adapters.adapter import (
    ApiAdapter,
    ApiAdapterResponse,
    request_types,
    response_types,
)
from tornado import escape
from tornado.ioloop import IOLoop

from odin_data.control.ipc_tornado_client import IpcTornadoClient
from odin_data.util import remove_prefix, remove_suffix


class OdinDataAdapter(ApiAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    ERROR_NO_RESPONSE = "No response from client, check it is running"
    ERROR_FAILED_TO_SEND = "Unable to successfully send request to client"
    ERROR_FAILED_GET = "Unable to successfully complete the GET request"
    ERROR_FAILED_PUT = "Unable to successfully complete the PUT request"
    ERROR_PUT_MISMATCH = "The size of parameter array does not match the number of clients"

    SUPPORTED_COMMANDS = ['reset_statistics', 'request_version', 'shutdown']

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        super(OdinDataAdapter, self).__init__(**kwargs)

        self._endpoint_arg = None
        self._endpoints = []
        self._clients = []
        self._client_connections = []
        self._update_interval = None
        self._config_file = []
        self._config_params = {}

        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]
        self._kwargs['module'] = self.name
        self._kwargs['api'] = 0.1

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
            logging.debug("Creating client {}:{}".format(ep['ip_address'], ep['port']))
            self._clients.append(IpcTornadoClient(ep['ip_address'], ep['port']))
            self._client_connections.append(False)
            self._config_file.append('')

        self._kwargs['count'] = len(self._clients)
        # Allocate the status list
        self._status = {'status/error': ''}

        # Setup the time between client update requests
        self._update_interval = float(self.options.get('update_interval', 0.5))
        self._kwargs['update_interval'] = self._update_interval
        self.update_loop()

    def set_error(self, err):
        # Record the error message into the status
        self._status['status/error'] = err

    def clear_error(self):
        # Clear the error message out of the status dict
        self._status['status/error'] = ''

    @request_types('application/json', 'application/vnd.odin-native')
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

        # Check if the adapter type is being requested
        request_command = path.strip('/')
        if not request_command:
            key_list = list(self._kwargs.keys())
            for client in self._clients:
                for key in client.parameters:
                    if key not in key_list:
                        key_list.append(key)
            response['value'] = key_list
        elif request_command in self._kwargs:
            logging.debug("Adapter request for ini argument: %s", request_command)
            response['value'] = self._kwargs[request_command]
        elif request_command in self._status:
            logging.debug("Adapter request for status value: %s", request_command)
            response['value'] = self._status[request_command]
        else:
            try:
                if 'client_error' in request_command:
                    request_command = request_command.replace('client_error', 'error')
                uri_items = request_command.split('/')
                logging.debug(uri_items)
                # Now we need to traverse the parameters looking for the request
                client_index = -1
                # Check to see if the URI finishes with an index
                # If it does then we are only interested in reading that single client
                try:
                    index = int(uri_items[-1])
                    if index >= 0:
                        # This is a valid index so remove the value from the URI
                        uri_items = uri_items[:-1]
                        # Set the client index for submitting config to
                        client_index = index
                        logging.debug("URI without index: %s", request_command)
                except ValueError:
                    # This is OK, there is simply no index provided
                    pass

                response_items = []
                if client_index == -1:
                    if request_command.startswith('config/config_file'):
                        # Special case for a config file.  Read back the current filename
                        response_items = self._config_file
                    else:
                        for client in self._clients:
                            response_items.append(OdinDataAdapter.traverse_parameters(client.parameters, uri_items))
                else:
                    if request_command.startswith('config/config_file'):
                        # Special case for a config file.  Read back the current filename
                        response_items = self._config_file[client_index]
                    else:
                        response_items = OdinDataAdapter.traverse_parameters(self._clients[client_index].parameters,
                                                                             uri_items)

                logging.debug(response_items)
                response['value'] = response_items
            except:
                logging.debug(OdinDataAdapter.ERROR_FAILED_GET)
                status_code = 503
                response['error'] = OdinDataAdapter.ERROR_FAILED_GET

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json', 'application/vnd.odin-native')
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

        # Clear any errors
        self.clear_error()

        request_command = path.strip('/')

        try:
            # Request should start with either config/ or command/
            if request_command.startswith("config/"):
                request_command = remove_prefix(request_command, "config/")  # Take the rest of the URI

                # Parse the parameters
                parameters = None
                try:
                    parameters = json.loads(str(escape.url_unescape(request.body)))
                except ValueError:
                    # If the body could not be parsed into an object it may be a simple string
                    parameters = str(escape.url_unescape(request.body))

                # Do not store this configuration if it is the config file
                if not request_command.startswith('config_file'):
                    # Check if the parameter is a dict, and if so then merge.  Otherwise replace the value
                    if request_command in self._config_params:
                        if isinstance(self._config_params[request_command], dict):
                            self._config_params[request_command].update(parameters)
                        else:
                            self._config_params[request_command] = parameters
                    else:
                        self._config_params[request_command] = parameters
                    logging.debug("Stored config items: %s", self._config_params)
                response, status_code = self.process_configuration(request_command, parameters)

                if self.require_version_check(request_command):
                    self.request_version()

            elif request_command.startswith("command/"):
                request_command = remove_prefix(request_command, "command/")  # Take the rest of the URI

                client_index, uri_items = self.parse_uri(request_command)
                # Check that the command is supported
                # For a command the uri_items object should always be length 1, the command
                if len(uri_items) > 1:
                    logging.debug(OdinDataAdapter.ERROR_FAILED_PUT)
                    status_code = 503
                    response['error'] = 'Invalid URI for command: {}'.format(request_command)
                else:
                    command = uri_items[0]
                    if command in self.SUPPORTED_COMMANDS:
                        # Create the IPC message and send to all clients
                        response, status_code = self.send_command_to_clients(command, client_index)
                    else:
                        logging.debug(OdinDataAdapter.ERROR_FAILED_PUT)
                        status_code = 503
                        response['error'] = 'Invalid command requested: {}'.format(command)

        except Exception as ex:
            self.set_error(str(ex))
            raise

        return ApiAdapterResponse(response, status_code=status_code)

    def parse_uri(self, request_command):
        # Split the request_command into a list
        uri_items = request_command.split('/')

        client_index = -1
        # Check to see if the URI finishes with an index
        try:
            index = int(uri_items[-1])
            if index >= 0:
                # This is a valid index so remove the value from the URI
                uri_items = uri_items[:-1]
                # Set the client index for submitting config to
                client_index = index
        except ValueError:
            # This is OK, there is simply no index provided
            pass
        return client_index, uri_items

    def process_configuration(self, request_command, parameters):
        status_code = 200
        response = {}
        logging.debug("Process configuration with URI: %s", request_command)
        client_index = -1
        # Check to see if the URI finishes with an index
        # eg hdf5/frames/0
        # If it does then we are only interested in setting that single client
        uri_items = request_command.split('/')
        # Check for an integer
        try:
            index = int(uri_items[-1])
            if index >= 0:
                # This is a valid index so remove the value from the URI
                request_command = remove_suffix(request_command, "/" + uri_items[-1])
                # Set the client index for submitting config to
                client_index = index
                logging.debug("URI without index: %s", request_command)
        except ValueError:
            # This is OK, there is simply no index provided
            pass

        if request_command == 'config_file':
            # Special case where we have been asked to load a config file to submit to clients
            # The config file should contain a JSON representation of a list of config dicts,
            # which will be sent one after the other as client messages
            config_file_path = parameters.strip('"')
            if client_index == -1:
                for index in range(0, len(self._clients)):
                    self._config_file[index] = config_file_path
            else:
                self._config_file[client_index] = config_file_path

            response, status_code = self.process_configuration_file(config_file_path, client_index)
        else:
            # Any other PUT values that do not contain paths to config files
            response, status_code = self.send_to_clients(request_command, parameters, client_index)
        return response, status_code

    def process_configuration_file(self, config_file_path, client_index):
        status_code = 200
        response = {}
        if config_file_path != '':
            logging.debug("Loading configuration file {}".format(config_file_path))
            try:
                with open(config_file_path) as config_file:
                    config_obj = json.load(config_file)
                    # Loop over the items in the object and send them all to the client(s)
                    for message in config_obj:
                        # for command in message:
                        logging.debug("Sending message: %s", message)
                        response, status_code = self.send_to_clients(None, message, client_index)
                        if status_code != 200:
                            return response, status_code

            except IOError as io_error:
                logging.error("Failed to open configuration file: {}".format(io_error))
                status_code = 503
                response = {'error': "Failed to open configuration file: {}".format(io_error)}
            except ValueError as value_error:
                logging.error("Failed to parse json config: {}".format(value_error))
                status_code = 503
                response = {'error': "Failed to parse json config: {}".format(value_error)}
        else:
            logging.info("Not loading configuration file from an empty path")
        return response, status_code

    def process_reconnection(self, client):
        # We have been notified that a client has reconnected.
        # Loop over all stored configuration, sending any that needs to be processed
        logging.debug("Processing reconnection for client: %d", client)
        # First load the configuration file
        self.process_configuration_file(self._config_file[client], client)

        # Now check all other stored parameters
        for request_command in self._config_params:
            parameters = self._config_params[request_command]
            # Check if the request ends in an index and is it a match for our client
            client_index = -1
            uri_items = request_command.split('/')
            # Check for an integer
            try:
                index = int(uri_items[-1])
                if index >= 0:
                    # This is a valid index so remove the value from the URI
                    request_command = remove_suffix(request_command, "/" + uri_items[-1])
                    # Set the client index for submitting config to
                    client_index = index
                    logging.debug("URI without index: %s", request_command)
            except ValueError:
                # This is OK, there is simply no index provided
                pass

            if client_index == -1 or client_index == client:
                if client_index == -1:
                    request_command = request_command + "/{}".format(client)
                logging.debug("Client index match for request: %s with parameters: %s", request_command, parameters)
                try:
                    self.process_configuration(request_command, parameters)
                except Exception as ex:
                    logging.error(ex)

        # Finally request version information
        self.request_version(client)

    def request_version(self, client_index=-1):
        logging.debug("Requesting version information from client index: %d", client_index)
        self.send_command_to_clients('request_version', client_index)

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

    def send_command_to_clients(self, command, client_index=-1):
        status_code = 200
        response = {}
        try:
            if client_index == -1:
                # We are sending the value to all clients
                for client in self._clients:
                    client.send_request(command)
            else:
                # A client index has been specified
                self._clients[client_index].send_request(command)
        except Exception as err:
            logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
            logging.error("Error: %s", err)
            status_code = 503
            response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
        return response, status_code

    def send_to_clients(self, request_command, parameters, client_index=-1):
        status_code = 200
        response = {}

        # Check if the parameters object is a list
        if isinstance(parameters, list):
            logging.debug("List of parameters provided: %s", parameters)
            # Check the length of the list matches the number of clients
            if len(parameters) != len(self._clients):
                status_code = 503
                response['error'] = OdinDataAdapter.ERROR_PUT_MISMATCH
            elif client_index != -1:
                # A list of items has been supplied but also an index has been specified
                logging.debug("URI contains an index but parameters supplied as a list")
                status_code = 503
                response['error'] = OdinDataAdapter.ERROR_PUT_MISMATCH
            else:
                # Loop over the clients and parameters, sending each one
                for client, param_set in zip(self._clients, parameters):
                    if param_set:
                        try:
                            command, parameters = OdinDataAdapter.uri_params_to_dictionary(request_command,
                                                                                           param_set)
                            client.send_configuration(parameters, command)
                        except Exception as err:
                            logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                            logging.error("Error: %s", err)
                            status_code = 503
                            response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}

        else:
            logging.debug("Single parameter set provided: %s", parameters)
            if client_index == -1:
                # We are sending the value to all clients
                command, parameters = OdinDataAdapter.uri_params_to_dictionary(request_command, parameters)
                for client in self._clients:
                    try:
                        client.send_configuration(parameters, command)
                    except Exception as err:
                        logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                        logging.error("Error: %s", err)
                        status_code = 503
                        response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
            else:
                # A client index has been specified
                try:
                    command, parameters = OdinDataAdapter.uri_params_to_dictionary(request_command, parameters)
                    self._clients[client_index].send_configuration(parameters, command)
                except Exception as err:
                    logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                    logging.error("Error: %s", err)
                    status_code = 503
                    response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
        return response, status_code

    @staticmethod
    def traverse_parameters(param_set, uri_items):
        try:
            item_dict = param_set
            for item in uri_items:
                item_dict = item_dict[item]
        except KeyError as ex:
            item_dict = None
        return item_dict

    @staticmethod
    def uri_params_to_dictionary(request_command, parameters):
        # Check to see if the request contains more than one item
        if request_command is not None:
            request_list = request_command.split('/')
            logging.debug("URI request list: %s", request_list)
            param_dict = {}
            command = None
            if len(request_list) > 1:
                # We need to create a dictionary structure that contains the request list
                current_dict = param_dict
                for item in request_list[1:-1]:
                    current_dict[item] = {}
                    current_dict = current_dict[item]

                current_dict[request_list[-1]] = parameters
                command = request_list[0]
            else:
                param_dict = parameters
                command = request_command
        else:
            param_dict = parameters
            command = request_command

        logging.debug("Command [%s] parameter dictionary: %s", command, param_dict)
        return command, param_dict

    def update_loop(self):
        """Handle background update loop tasks.
        This method handles background update tasks executed periodically in the tornado
        IOLoop instance. This includes requesting the status from the underlying application
        and preparing the JSON encoded reply in a format that can be easily parsed.
        """
        logging.debug("Updating status from client...")

        # Handle background tasks
        # Loop over all connected clients and obtain the status
        index = 0
        for client in self._clients:
            try:
                # First check for stale status within a client (1 seconds)
                #client.check_for_stale_status(1.0)
                # Now check for a transition from disconnected to connected
                if not client.connected():
                    self._client_connections[index] = False
                else:
                    if not self._client_connections[index]:
                        self._client_connections[index] = True
                        # Reconnection event so push configuration
                        logging.debug("Client reconnection event")
                        self.process_reconnection(index)

            except Exception as e:
                # Exception caught, log the error but do not stop the update loop
                logging.error("Unhandled exception: %s", e)

            # Request parameter updates
            for parameter_tree in ["status", "request_configuration"]:
                try:
                    client.send_request(parameter_tree)
                except Exception as e:
                    # Log the error, but do not stop the update loop
                    logging.error("Unhandled exception: %s", e)

            index += 1

            self.process_updates()

        # Schedule the update loop to run in the IOLoop instance again after appropriate interval
        IOLoop.instance().call_later(self._update_interval, self.update_loop)

    def require_version_check(self, parameter):
        """Check if a version request is required after the configuration parameter has been submitted.

        Child classes can implement logic here to force a version request.

        """
        return False

    def process_updates(self):
        """Handle additional background update loop tasks

        Child classes can implement logic here to take any action based on the
        latest parameter tree, before the next update is scheduled.

        """
        pass
