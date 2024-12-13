import logging

from odin.adapters.adapter import ApiAdapterResponse, request_types, response_types
from tornado import escape

from odin_data.control.odin_data_adapter import OdinDataAdapter


class MetaListenerAdapter(OdinDataAdapter):

    """An OdinControl adapter for a MetaListener"""

    def __init__(self, **kwargs):
        logging.debug("MetaListenerAdapter init called")

        # These are internal adapter parameters
        self.acquisition_id = None
        self.acquisition_active = False
        self.acquisitions = []
        # These parameters are stored under an acquisition tree, so we need to
        # parse out the parameters for the acquisition we have stored
        self._status_parameters = {}
        self._set_defaults()
        # These config parameters are buffered so they can be included whenever a new acquisition
        # is created. This helps to abstract the idea of acquisitions being created and removed and
        # means the client does not need to send things in a certain order.
        self._config_parameters = {
            "config/directory": "",
            "config/file_prefix": "",
            "config/flush_frame_frequency": 100,
            "config/flush_timeout": 1,
        }

        # Parameters must be created before base init called
        super(MetaListenerAdapter, self).__init__(**kwargs)
        self._client = self._controller._clients[0]  # We only have one client
        self._controller.process_updates = self.process_updates

    def _set_defaults(self):
        self._status_parameters = {
            "status/full_file_path": "",
            "status/num_processors": 0,
            "status/writing": False,
            "status/written": 0
        }

    def _map_acquisition_parameter(self, path):
        """Map acquisition parameter path string to full uri item list"""
        # Replace the first slash with acquisitions/<acquisition_id>/
        # E.g. status/filename -> status/acquisitions/<acquisition_id>/filename
        full_path = path.replace(
            "/", "/acquisitions/{}/".format(self.acquisition_id),
            1  # First slash only
        )
        return full_path.split("/")  # Return list of uri items

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def get(self, path, request):

        """Implementation of the HTTP GET verb for MetaListenerAdapter

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client

        """
        status_code = 200
        response = {}
        logging.debug("GET path: %s", path)
        logging.debug("GET request: %s", request)

        if path == "config/acquisition_id":
            response["acquisition_id"] = self.acquisition_id
        elif path == "status/acquisition_active":
            response["acquisition_active"] = self.acquisition_active
        elif path == "config/acquisitions":
            acquisition_tree = self.traverse_parameters(
                self._clients[0].parameters,
                ["config", "acquisitions"]
            )
            if acquisition_tree is not None:
                response["acquisitions"] = "," .join(acquisition_tree.keys())
            else:
                response["acquisitions"] = None
        elif path in self._status_parameters:
            response[path.split("/")[-1]] = self._status_parameters[path]
        elif path in self._config_parameters:
            response[path.split("/")[-1]] = self._config_parameters[path]
        else:
            return super(MetaListenerAdapter, self).get(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def put(self, path, request):

        """
        Implementation of the HTTP PUT verb for MetaListenerAdapter

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client

        """
        logging.debug("PUT path: %s", path)
        logging.debug("PUT request: %s", request)
        logging.debug("PUT request.body: %s",
                      str(escape.url_unescape(request.body)))

        value = str(escape.url_unescape(request.body)).replace('"', '')

        if path in self._config_parameters:
            # Store config to re-send with acquisition ID when it is changed
            self._config_parameters[path] = value

        if path == "config/acquisition_id":
            self.acquisition_id = value
            # Set inactive so process_updates doesn't clear acquisition ID
            self.acquisition_active = False
            # Send entire config with new acquisition ID
            config = dict(acquisition_id=self.acquisition_id)
            for key, value in self._config_parameters.items():
                # Add config parameters without config/ prefix
                config[key.split("config/")[-1]] = value
            status_code, response = self._send_config(config)
        elif path == "config/stop":
            self.acquisition_active = False

            # By default we stop all acquisitions by passing None
            config = {
                "acquisition_id": None,
                "stop": True
            }
            if self.acquisition_id is not None:
                # If we have an Acquisition ID then stop that one only
                config["acquisition_id"] = self.acquisition_id
            status_code, response = self._send_config(config)

            self.acquisition_id = None
        elif self.acquisition_id is not None:
            parameter = path.split("/", 1)[-1]  # Remove 'config/'
            config = {"acquisition_id": self.acquisition_id, parameter: value}
            status_code, response = self._send_config(config)
        else:
            return super(MetaListenerAdapter, self).put(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    def _send_config(self, config_message):
        status_code = 200
        response = {}
        try:
            self._client.send_configuration(config_message)
        except Exception as err:
            logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
            logging.error("Error: %s", err)
            status_code = 503
            response = {"error": OdinDataAdapter.ERROR_FAILED_TO_SEND}

        return status_code, response

    def process_updates(self):
        """Handle additional background update loop tasks

        Store a copy of all parameters so they don't disappear

        """
        if self.acquisition_id is not None:
            acquisitions = self.traverse_parameters(
                self._client.parameters, ["status", "acquisitions"]
            )
            acquisition_active = (
                acquisitions is not None and self.acquisition_id in acquisitions
            )
            if acquisition_active:
                self.acquisition_active = True
                for parameter in self._status_parameters:
                    value = self.traverse_parameters(
                        self._client.parameters,
                        self._map_acquisition_parameter(parameter)
                    )
                    self._status_parameters[parameter] = value
            else:
                self.acquisition_active = False
                self._set_defaults()
        else:
            self._set_defaults()
