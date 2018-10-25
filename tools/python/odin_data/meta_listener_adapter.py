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
        self._acquisition_parameters = [
            "status/filename",
            "status/num_processors",
            "status/writing",
            "status/written",
            "config/output_dir",
            "config/flush"
        ]

        # Parameters must be created before base init called
        super(MetaListenerAdapter, self).__init__(**kwargs)
        self._client = self._clients[0]  # We only have one client

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
        elif path == "config/acquisitions":
            acquisition_tree = self.traverse_parameters(
                self._clients[0].parameters,
                ["config", "acquisitions"]
            )
            response["value"] = "," .join(acquisition_tree.keys())
        elif path in self._acquisition_parameters:
            if self.acquisitionID:
                response["value"] = self.traverse_parameters(
                    self._client.parameters,
                    self._map_acquisition_parameter(path)
                )
            elif path == "status/writing":
                # Must return False, not None, if acquisition does not exist
                response["value"] = False
            else:
                response["value"] = None
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
        status_code = 200
        response = {}
        logging.error("PUT path: %s", path)
        logging.error("PUT request: %s", request)
        logging.error("PUT request.body: %s",
                      str(escape.url_unescape(request.body)))

        value = str(escape.url_unescape(request.body)).replace('"', '')

        if path == "config/acquisition_id":
            self.acquisitionID = value
            # Set inactive so process_updates doesn't clear acquisition ID
            self.acquisition_active = False
            # Send the PUT request
            try:
                config = {
                    "acquisition_id": self.acquisitionID,
                }
                self._client.send_configuration(config)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)
                status_code = 503
                response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
        elif path == "config/stop":
            # By default we stop all acquisitions by passing None
            config = {
                "acquisition_id": None,
                "stop": True
            }
            if self.acquisitionID:
                # If we have an Acquisition ID then stop that one only
                config["acquisition_id"] = self.acquisitionID
            try:
                self._client.send_configuration(config)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)
                status_code = 503
                response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
            self.acquisitionID = ""
            self.acquisition_active = False
        elif path in self._acquisition_parameters:
            # Send the PUT request on with the acquisitionID attached
            try:
                parameter = path.split("/", 1)[-1]  # Remove 'config/'
                config = {
                    "acquisition_id": self.acquisitionID,
                    parameter: value
                }
                self._client.send_configuration(config)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)
                status_code = 503
                response = {'error': OdinDataAdapter.ERROR_FAILED_TO_SEND}
        else:
            return super(OdinDataAdapter, self).put(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    def process_updates(self):
        """Handle additional background update loop tasks

        Reset acquisitionID if it finishes writing.

        """
        if self.acquisitionID:
            currently_writing = self.traverse_parameters(
                self._clients[0].parameters,
                ["status", "acquisitions", self.acquisitionID, "writing"]
            )
            if self.acquisition_active and not currently_writing:
                self.acquisitionID = ""
            self.acquisition_active = currently_writing
