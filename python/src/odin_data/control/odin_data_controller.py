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


def setter_func(tornado_client:IpcTornadoClient, path: list, value):
    if(value is not None):
        tornado_client.send_configuration({path[-1], value})

def getter_func(paramTree: dict, path: list):
    return reduce(lambda d, key: d[key], path, paramTree)

class OdinDataController(object):
    def __init__(self, name, endpoints, update_interval=0.5):
        self._clients = []
        self._client_connections = []
        self._update_interval = update_interval
        self._name = name
        self._api = 0.1
        self._error = ""
        self.config_ts:list[int] = [-1] * len(endpoints)
        self.status_ts:list[int] = [-1] * len(endpoints)
        self.config_ts_prev:list[int] = [0] * len(endpoints)
        self.status_ts_prev:list[int] = [0] * len(endpoints)
        self._endpoints = []
        self._command_needs_update: bool = False
        self._config_resposes:list[dict] = [None] * len(endpoints)
        self._status_resposes:list[dict] = [None] * len(endpoints)

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
        # Record the error message into the status
        self._error = err

    def clear_error(self):
        # Clear the error message out of the status dict
        self._error = ""

    def get(self, path, meta):
        """
        Return the ParameterTree value for the supplied path

        :param path: URI path of the GET request
        :param meta: Should the ParameterTree return the meta data associated with the value
        :return: dict object containing the value and meta data if requested
        """
        return self._params.get(path, meta) # ParameterTree.get() returns the value in the cache

    def put(self, path, value):
        self._params.set(path, value)
        # After all config processing has completed, execute queued commands
        # self.execute_queued() # Still necessary??
    
    def recursive_splice(self, index:int, resp_type:str, path:list, params_node:dict, metadata:dict):
        if isinstance(params_node, dict):
            return {
                k: self.recursive_splice(index, resp_type, path + [k], v, metadata) for k, v in params_node.items()
            }
        else:
            try:
                param_metadata = reduce(lambda d, key: d[key], path, metadata)
                setter = None
                if(resp_type == IpcTornadoClient.CONFIG_PARAMS_KEY):
                    getter = partial(getter_func, self._config_resposes[index], path)
                else:
                    getter = partial(getter_func, self._status_resposes[index], path)
                metadata = dict(param_metadata)
                if(metadata["access_mode"] == "rw"): # has to be a configuration parameter! So we assign a setter!
                    setter = partial(setter_func, self._clients[index], path)
                metadata.pop("access_mode", None) # pop "access_mode"
                metadata.pop(ParameterAccessor.AUTO_METADATA_FIELDS[0], None) # pop "type"
                return (getter, setter, metadata)
            except (KeyError, TypeError):
                # Safe fallback: keeps the data, flags missing metadata
                return (params_node, None)

    def splice_params_metadata(self, index, resp_type:str, params:dict, metadata:dict):
        """
        Recursive function to append metadata of each parameter to it's value together in a tuple.
        This is the format the ParameterTree expects it.
        index - the index of the IpcTornadoClient object.
        resp_type - string to indicate if it is a STATUS/CONFIG response
        params_node - Config/Status Parameters dictionary.
        metadata - Config/Status Parameter-metadata dictionary.
        """
        path = [] # path - the full path of the parameter is a list format.
        params = self.recursive_splice(index, resp_type, path, params, metadata)
        return params

    def _update_params_with_metadata(self, value_dict:dict, index:int, param_key:str, metadata_key:str, metadata_ts_key:str):
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
        if(param_key in value_dict):
            resp = value_dict[param_key]
            if(param_key == IpcTornadoClient.CONFIG_PARAMS_KEY):
                self._config_resposes[index] = resp
                pt_string = IpcTornadoClient.IPC_VAL_CONFIG
            elif(param_key == IpcTornadoClient.STATUS_PARAMS_KEY):
                self._status_resposes[index] = resp
                pt_string = IpcTornadoClient.IPC_VAL_STATUS
        if(metadata_ts_key in value_dict):
            response_ts_ver = value_dict[metadata_ts_key]
        if metadata_key in value_dict:
            metadata = value_dict[metadata_key]
            # If we have received metadata then we need to update the 'command' structure on the next processing.
            # Because the 'command" structure might have an update!
            self._command_needs_update = True
            if(resp is not None):
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
                            if(param_req == IpcTornadoClient.IPC_VAL_REQ_CFG and self.config_ts[index] != self.config_ts_prev[index]):
                                with_metadata = True
                                self.config_ts_prev[index] = self.config_ts[index]
                            elif(param_req == IpcTornadoClient.IPC_VAL_STATUS and self.status_ts[index] != self.status_ts_prev[index]):
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
                    if IpcTornadoClient.IPC_VAL_STATUS in client.parameters and \
                        client.parameters[IpcTornadoClient.IPC_VAL_STATUS][IpcTornadoClient.STATUS_PARAMS_KEY][IpcTornadoClient.CLIENT_CONNECTED]:
                        self.status_ts[index] = self._update_params_with_metadata(client.parameters[IpcTornadoClient.IPC_VAL_STATUS],
                                                                                        index, IpcTornadoClient.STATUS_PARAMS_KEY,
                                                                                        IpcTornadoClient.IPC_VAL_STATUS_METADATA,
                                                                                        IpcTornadoClient.IPC_VAL_STATUS_TS)
                    if IpcTornadoClient.IPC_VAL_CONFIG in client.parameters:
                        self.config_ts[index] = self._update_params_with_metadata(client.parameters[IpcTornadoClient.IPC_VAL_CONFIG],
                                                                                        index, IpcTornadoClient.CONFIG_PARAMS_KEY,
                                                                                        IpcTornadoClient.IPC_VAL_CONFIG_METADATA,
                                                                                        IpcTornadoClient.IPC_VAL_CONFIG_TS)
                    if "commands" in client.parameters and (self._command_needs_update == True):
                        self.parse_available_commands(index, client)
                        self._command_needs_update = False
                self.process_updates()
            except Exception as ex:
                logging.error("{}".format(ex))

            time.sleep(self._update_interval)

    def parse_available_commands(self, index, client):
        # Check for differences in the command structure
        logging.debug(
            f"Command structure has changed: {client.parameters['commands']}"
        )
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
                    lambda value, index=index, plugin=plugin: self.queue_command(
                        index, plugin, value
                    ),
                    {},
                ),
            }
        # If the structure has changed then update the parameter tree
            self._params.replace(f"{index}/command", command_tree)
        self._supported_commands[index] = client.parameters["commands"]

    def queue_command(self, index, plugin, value):
        """Called for each command PUT that is received by the adapter
        PUT URI is of the form index/command/plugin/execute and the
        value is the name of the command to execute.
        This method simply queues commands for execution after any configuration
        changes have been applied.
        """
        logging.info(
            f"Queue command: index [{index}] plugin [{plugin}] command [{value}]"
        )
        self._queued_command[index] = (plugin, value)

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
