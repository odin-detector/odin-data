"""
Created on 30th November 2023

:author: Alan Greer
"""

import logging
import threading
import time
from functools import partial, reduce

from odin.adapters.parameter_tree import ParameterAccessor, ParameterTree

from odin_data.control.ipc_tornado_client import IpcTornadoClient


def setter_func(tornado_client: IpcTornadoClient, path: list, value):
    """
    Send a configuration message to a C++ odin-data application with the parameter name
    specified by the path and the value set to value

    :param tornado_client: Client connection to C++ odin-data application.
    :param path: List of path components of the configuration parameter to set.
    :param value: New value of the configuration parameter.
    """
    if value is not None:
        tornado_client.send_configuration({path[-1], value})


def getter_func(paramTree: dict, path: list):
    """
    Walks down through the `paramTree` dict by following a sequence of keys specified in `path`.
    Each element of `path` is used to index into the current level, descending one level per key.

    :param paramTree: The dictionary to search.
    :param path: Sequence of keys used to walk down through the dictionary.
    :return: The value found at the given path.
    """
    return reduce(lambda d, key: d[key], path, paramTree)


class OdinDataController(object):
    def __init__(self, name, endpoints, update_interval=0.5):
        """
        Initialise an OdinDataController object.  Record the client connection endpoints and
        set up corresponding `IpcTornadoClient`s for each endpoint.  Set up the initial `ParameterTree`
        for the controller and create a status thread for continual monitoring of the client
        applications status and configuration.

        :param name: Set the name of this controller object.
        :param endpoints: A list of endpoint strings for clients in the form `<ip_address>:<port>`.
        :param update_interval: The status update interval in seconds.
        """
        self._clients = []
        self._client_connections = []
        self._update_interval = update_interval
        self._name = name
        self._api = 0.1
        self._error = ""
        self.config_ts: list[int] = [-1] * len(endpoints)
        self.status_ts: list[int] = [-1] * len(endpoints)
        self.config_ts_prev: list[int] = [0] * len(endpoints)
        self.status_ts_prev: list[int] = [0] * len(endpoints)
        self._endpoints = []
        self._command_needs_update: bool = False
        self._config_responses: list[dict | None] = [None] * len(endpoints)
        self._status_responses: list[dict | None] = [None] * len(endpoints)

        for arg in endpoints.split(","):
            arg = arg.strip()
            logging.debug("Endpoint: %s", arg)
            ep = {"ip_address": arg.split(":")[0], "port": int(arg.split(":")[1])}
            self._endpoints.append(ep)

        self._supported_commands = [None] * len(self._endpoints)
        self._queued_command = [None] * len(self._endpoints)

        for ep in self._endpoints:
            logging.debug("Creating client {}:{}".format(ep["ip_address"], ep["port"]))
            self._clients.append(IpcTornadoClient(ep["ip_address"], ep["port"]))
            self._client_connections.append(False)

        # set up controller specific parameters
        self.setup_parameter_tree()

        self._params = ParameterTree(self._tree, mutable=True)

        # Create the status loop handling thread
        self._status_running = True
        self._status_lock = threading.Lock()
        self._status_thread = threading.Thread(target=self.update_loop)
        self._status_thread.start()

    def setup_parameter_tree(self):
        """
        Builds the initial `ParameterTree` dictionary for this `OdinDataController`.  Adds top-level
        information including the API version and module name, as well as the client list.  Then
        for each connected client `status`, `config`, and `command` dictionaries are added.  These are
        all empty at this stage, and are filled by introspecting the client applications once connected.
        """
        self._tree = {
            "api": (lambda: self._api, None, {}),
            "module": (lambda: self._name, None, {}),
            "endpoints": [],
            "count": (lambda: len(self._clients), None, {}),
            "update_interval": (lambda: self._update_interval, None, {}),
        }
        for idx, endpoint in enumerate(self._endpoints):
            self._tree["endpoints"].append(
                # Note the default here binds unique variables into each closure
                {k: (lambda v=v: v, None, {}) for k, v in endpoint.items()}
            )
        for idx, _client in enumerate(self._clients):
            self._tree[str(idx)] = {
                "status": {"error": (lambda: self._error, None, {})},
                "config": {},
                "command": {},
            }

    def set_error(self, err):
        """
        Record the error message into the status of this controller's `ParameterTree`.

        :param err: The error message to set.
        """
        self._error = err

    def clear_error(self):
        """
        Clear the error message out of the status of this controller's `ParameterTree`.
        """
        self._error = ""

    def get(self, path, meta):
        """
        Return the ParameterTree value for the supplied path

        :param path: URI path of the GET request
        :param meta: Should the ParameterTree return the meta data associated with the value
        :return: dict object containing the value and meta data if requested
        """
        return self._params.get(
            path, meta
        )  # ParameterTree.get() returns the value in the cache

    def put(self, path, value):
        """
        Sets `value` at the provided `path` on the internal `ParameterTree`.  This will result
        in a callback triggering through the `setter_func` module function and send the value
        to the underlying C++ applications.

        :params path: URI path of the parameter to set.
        :param value: Value to set.
        """
        self._params.set(path, value)
        # After all config processing has completed, execute queued commands
        # self.execute_queued() # Still necessary??

    def recursive_splice(
        self, index: int, resp_type: str, path: list, params_node: dict, metadata: dict
    ):
        """
        Recursively walks the supplied `params_node` dictionary, and for each leaf value looks
        up matching metadata and builds a `(getter, setter, metadata)` tuple suitable for the
        `ParameterTree`.  A setter is only added when the access mode of the `metadata` is "rw".

        :param index: The index of the IpcTornadoClient object.
        :param resp_type: String to indicate if it is a STATUS/CONFIG response.
        :param params_node: Config/Status parameters dictionary.
        :param metadata: Config/Status parameter metadata dictionary.
        :return: tuple of `(getter, setter, metadata)`
        """
        if isinstance(params_node, dict):
            return {
                k: self.recursive_splice(index, resp_type, path + [k], v, metadata)
                for k, v in params_node.items()
            }
        else:
            try:
                param_metadata = reduce(lambda d, key: d[key], path, metadata)
                setter = None
                if resp_type == IpcTornadoClient.CONFIG_PARAMS_KEY:
                    getter = partial(getter_func, self._config_responses[index], path)
                else:
                    getter = partial(getter_func, self._status_responses[index], path)
                metadata = dict(param_metadata)
                if (
                    metadata["access_mode"] == "rw"
                ):  # has to be a configuration parameter! So we assign a setter!
                    setter = partial(setter_func, self._clients[index], path)
                metadata.pop("access_mode", None)  # pop "access_mode"
                metadata.pop(
                    ParameterAccessor.AUTO_METADATA_FIELDS[0], None
                )  # pop "type"
                return (getter, setter, metadata)
            except (KeyError, TypeError):
                # Safe fallback: keeps the data, flags missing metadata
                return (params_node, None)

    def splice_params_metadata(
        self, index, resp_type: str, params: dict, metadata: dict
    ):
        """
        Recursive function to append metadata of each parameter to it's value together in a tuple.
        This is the format the ParameterTree expects it.
        index - the index of the IpcTornadoClient object.
        resp_type - string to indicate if it is a STATUS/CONFIG response
        params_node - Config/Status Parameters dictionary.
        metadata - Config/Status Parameter-metadata dictionary.
        """
        path = []  # path - the full path of the parameter is a list format.
        params = self.recursive_splice(index, resp_type, path, params, metadata)
        return params

    def _update_params_with_metadata(
        self,
        value_dict: dict,
        index: int,
        param_key: str,
        metadata_key: str,
        metadata_ts_key: str,
    ):
        """
        dict - The dictionary containing the status or config_request response.
        index - The index of the IpcClient which received this response. This is used as a key in the ParameterTree
        param_key - The key which the IpcTornadoClient stored the response in; either STATUS_PARAM_KEY/CONFIG_PARAM_KEY
        metadata_key - The key used by IpcTornado CLient to store the metadata. This corresponds to either: STATUS_METADATA/CONFIG_METADATA
        metadata_ts_key - The key used by IpcTornado CLient to store the metadata timestamp. Corresponds to either STATUS_METADATA_TS/CONFIG_METADATA_TS
        """
        resp = None
        response_ts_ver = 0
        pt_string = ""
        if param_key in value_dict:
            resp = value_dict[param_key]
            if param_key == IpcTornadoClient.CONFIG_PARAMS_KEY:
                self._config_responses[index] = resp
                pt_string = IpcTornadoClient.IPC_VAL_CONFIG
            elif param_key == IpcTornadoClient.STATUS_PARAMS_KEY:
                self._status_responses[index] = resp
                pt_string = IpcTornadoClient.IPC_VAL_STATUS
        if metadata_ts_key in value_dict:
            response_ts_ver = value_dict[metadata_ts_key]
        if metadata_key in value_dict:
            metadata = value_dict[metadata_key]
            # If we have received metadata then we need to update the 'command' structure on the next processing.
            # Because the 'command" structure might have an update!
            self._command_needs_update = True
            if resp is not None:
                resp = self.splice_params_metadata(index, param_key, resp, metadata)
                # Rebuild the entire tree
                self._params.replace(f"{index}/{pt_string}", resp)
        return response_ts_ver

    def update_loop(self):
        """Handle background update loop tasks.
        This method handles background update tasks executed periodically in the tornado
        IOLoop instance. This includes requesting the status from the underlying application
        and preparing the JSON encoded reply in a format that can be easily parsed.
        """
        logging.debug("Starting the status/config update thread...")

        while self._status_running:
            try:
                # Handle background tasks
                # Loop over all connected clients and obtain the status
                for index, client in enumerate(self._clients):
                    try:
                        # First check for stale status within a client (1 seconds)
                        # client.check_for_stale_status(1.0)
                        # Now check for a transition from disconnected to connected
                        if not client.connected():
                            self._client_connections[index] = False
                        else:
                            if not self._client_connections[index]:
                                self._client_connections[index] = True
                    except Exception as e:
                        # Exception caught, log the error but do not stop the update loop
                        logging.error("Unhandled exception: %s", e)
                    # Request parameter updates
                    for param_req in [
                        IpcTornadoClient.IPC_VAL_STATUS,
                        IpcTornadoClient.IPC_VAL_REQ_CFG,
                        IpcTornadoClient.IPC_VAL_REQ_CMDS,
                    ]:
                        try:
                            with_metadata = False
                            # Check if the previous values of the config and status time-stamp matches the latest value.
                            # If they do not match, set 'with_metadata' to True and update the previous time-stamp value with the latest.
                            if (
                                param_req == IpcTornadoClient.IPC_VAL_REQ_CFG
                                and self.config_ts[index] != self.config_ts_prev[index]
                            ):
                                with_metadata = True
                                self.config_ts_prev[index] = self.config_ts[index]
                            elif (
                                param_req == IpcTornadoClient.IPC_VAL_STATUS
                                and self.status_ts[index] != self.status_ts_prev[index]
                            ):
                                with_metadata = True
                                self.status_ts_prev[index] = self.status_ts[index]
                            msg = client.send_request(param_req, with_metadata)
                            if client.wait_for_response(msg.get_msg_id()):
                                logging.error(
                                    f"{param_req} request to "
                                    f"{client.ctrl_endpoint} timed out"
                                )
                        except Exception as e:
                            # Log the error, but do not stop the update loop
                            logging.error("Unhandled exception: %s", e)
                    self.handle_client(client, index)
                    # Always track/update the time-stamp values of config and status
                    # using the class members status_ts & config_ts variables
                    if (
                        IpcTornadoClient.IPC_VAL_STATUS in client.parameters
                        and client.parameters[IpcTornadoClient.IPC_VAL_STATUS][
                            IpcTornadoClient.STATUS_PARAMS_KEY
                        ][IpcTornadoClient.CLIENT_CONNECTED]
                    ):
                        self.status_ts[index] = self._update_params_with_metadata(
                            client.parameters[IpcTornadoClient.IPC_VAL_STATUS],
                            index,
                            IpcTornadoClient.STATUS_PARAMS_KEY,
                            IpcTornadoClient.IPC_VAL_STATUS_METADATA,
                            IpcTornadoClient.IPC_VAL_STATUS_TS,
                        )
                    if IpcTornadoClient.IPC_VAL_CONFIG in client.parameters:
                        self.config_ts[index] = self._update_params_with_metadata(
                            client.parameters[IpcTornadoClient.IPC_VAL_CONFIG],
                            index,
                            IpcTornadoClient.CONFIG_PARAMS_KEY,
                            IpcTornadoClient.IPC_VAL_CONFIG_METADATA,
                            IpcTornadoClient.IPC_VAL_CONFIG_TS,
                        )
                    if "commands" in client.parameters and (
                        self._command_needs_update == True
                    ):
                        self.parse_available_commands(index, client)
                        self._command_needs_update = False
                self.process_updates()
            except Exception as ex:
                logging.error("{}".format(ex))

            time.sleep(self._update_interval)

    def parse_available_commands(self, index, client):
        """
        Rebuilds the `command` branch of the `ParameterTree` from the client's currently
        supported commands.

        :param index: The index of the IpcTornadoClient object.
        :param client: The IpcTornadoClient.
        """
        # Check for differences in the command structure
        logging.debug(f"Command structure has changed: {client.parameters['commands']}")
        command_tree = {}
        for plugin in client.parameters["commands"]:
            # Build the execution branch for each plugin
            command_tree[plugin] = {
                "allowed": (
                    lambda x=client.parameters["commands"][plugin]["supported"]: x,
                    None,
                    {},
                ),
                "execute": (
                    "",
                    lambda value, index=index, plugin=plugin: self.send_command(
                        index, plugin, value
                    ),
                    {},
                ),
            }
            # If the structure has changed then update the parameter tree
            self._params.replace(f"{index}/command", command_tree)
        self._supported_commands[index] = client.parameters["commands"]

    def send_command(self, index, plugin, value):
        """Called for each command from parse_available_commands
        PUT URI is of the form index/command/plugin/execute and the
        value is the name of the command to execute.
        This method sends each commands as demanded.
        """
        logging.info(
            f"Send command: index [{index}] plugin [{plugin}] command [{value}]"
        )
        # Call the execution check method prior to sending to a client
        if self.can_execute(index, plugin, value):
            self._clients[index].execute_command(plugin, value)

    def can_execute(self, index, plugin, command):
        """Called for each command that is about to be sent to a client
        application.  If this method returns false then the command is not
        sent.  This method can be overloaded by subclasses to implement
        controller specific logic checks prior to sending the command.
        """
        return True

    def handle_client(self, client, index):
        """Called on each client in the update_loop loop before updating the
        parameter tree and caching the config, can be overloaded by
        subclasses to implement controller specific logic.
        """
        pass

    def create_demand_config(self, new_params, old_params):
        config = None
        for item in new_params:
            logging.debug("Param: {}".format(item))
            logging.debug("   Type: {}".format(type(new_params[item])))
            if item in old_params:
                if isinstance(new_params[item], dict):
                    diff = self.create_demand_config(new_params[item], old_params[item])
                    if diff is not None:
                        if config is None:
                            config = {}
                        config[item] = diff
                elif isinstance(new_params[item], list):
                    if config is None:
                        config = {item: []}
                    for new_item, old_item in zip(new_params[item], old_params[item]):
                        if isinstance(new_item, dict):
                            config[item].append(
                                self.create_demand_config(new_item, old_item)
                            )
                        else:
                            if new_item != old_item:
                                config[item].append(new_item)
                else:
                    if new_params[item] != old_params[item]:
                        if config is None:
                            config = {}
                        config[item] = new_params[item]
        return config

    def process_updates(self):
        """Handle additional background update loop tasks

        Child classes can implement logic here to take any action based on the
        latest parameter tree, before the next update is scheduled.

        """
        pass

    def shutdown(self):
        self._status_running = False

    def _set(self, attr, val):
        logging.debug("_set called: {}  {}".format(attr, val))
        setattr(self, attr, val)

    def _get(self, attr):
        return lambda: getattr(self, attr)
