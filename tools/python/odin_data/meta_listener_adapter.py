import logging

from tornado import escape

from odin.adapters.adapter import ApiAdapterResponse, \
    request_types, response_types
from odin_data.odin_data_adapter import OdinDataAdapter


class MetaListenerAdapter(OdinDataAdapter):

    """An OdinControl adapter for a MetaListener"""

    def __init__(self, **kwargs):
        logging.debug("MetaListenerAdapter init called")

        # These are internal adapter parameters
        self.acquisitionID = ""
        self.acquisition_active = False
        self.acquisitions = []
        # These parameters are stored under an acquisition tree, so we need to
        # parse out the parameters for the acquisition we have stored
        self._acquisition_parameters = {}
        self._set_defaults()

        # Parameters must be created before base init called
        super(MetaListenerAdapter, self).__init__(**kwargs)
        self._client = self._clients[0]  # We only have one client

    def _set_defaults(self):
        self.acquisitionID = ""
        self._acquisition_parameters = {
            "status/filename": "",
            "status/num_processors": 0,
            "status/writing": False,
            "status/written": 0,
            "config/output_dir": "",
            "config/flush": 1
        }

    def _map_acquisition_parameter(self, path):
        """Map acquisition parameter path string to full uri item list"""
        # Replace the first slash with acquisitions/<acquisitionID>/
        # E.g. status/filename -> status/acquisitions/<acquisitionID>/filename
        full_path = path.replace(
            "/", "/acquisitions/{}/".format(self.acquisitionID),
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
            response["value"] = self.acquisitionID
        elif path == "status/acquisition_active":
            response["value"] = self.acquisition_active
        elif path == "config/acquisitions":
            acquisition_tree = self.traverse_parameters(
                self._clients[0].parameters,
                ["config", "acquisitions"]
            )
            if acquisition_tree is not None:
                response["value"] = "," .join(acquisition_tree.keys())
            else:
                response["value"] = None
        elif path in self._acquisition_parameters:
            response["value"] = self._acquisition_parameters[path]
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

        if path == "config/acquisition_id":
            self.acquisitionID = value
            # Set inactive so process_updates doesn't clear acquisition ID
            self.acquisition_active = False

            config = {
                "acquisition_id": self.acquisitionID,
            }
            status_code, response = self._send_config(config)
        elif path == "config/stop":
            self.acquisitionID = ""
            self.acquisition_active = False
            self._set_defaults()

            # By default we stop all acquisitions by passing None
            config = {
                "acquisition_id": None,
                "stop": True
            }
            if self.acquisitionID:
                # If we have an Acquisition ID then stop that one only
                config["acquisition_id"] = self.acquisitionID
            status_code, response = self._send_config(config)
        elif path in self._acquisition_parameters:
            # Send the PUT request on with the acquisitionID attached
            parameter = path.split("/", 1)[-1]  # Remove 'config/'
            config = {
                "acquisition_id": self.acquisitionID,
                parameter: value
            }
            status_code, response = self._send_config(config)
        else:
            return super(OdinDataAdapter, self).put(path, request)

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
        if self.acquisitionID:
            acquisition_active = self.acquisitionID in self.traverse_parameters(
                self._client.parameters, ["status", "acquisitions"]
            )
            if acquisition_active:
                self.acquisition_active = True
                for parameter in self._acquisition_parameters.keys():
                    value = self.traverse_parameters(
                        self._client.parameters,
                        self._map_acquisition_parameter(parameter)
                    )
                    if parameter.startswith("config/") and value is None:
                        continue  # Work around config being set to None on last poll of acquisition
                    self._acquisition_parameters[parameter] = value
            else:
                self.acquisition_active = False
        else:
            self._set_defaults()
