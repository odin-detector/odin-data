"""
Created on 30th November 2023

:author: Alan Greer
"""

import logging
import threading
import time
from functools import partial, reduce

from deepdiff import DeepDiff
from odin.adapters.parameter_tree import ParameterAccessor, ParameterTree

from odin_data.control.ipc_tornado_client import IpcTornadoClient


def setter_func(tornado_client:IpcTornadoClient, path: list, value):
    if(path[-1] == ''):
        del path[-1]
    if(value is not None):
        tornado_client.send_configuration({path[-1], value})

def getter_func(paramTree: dict, path: list):
    if(path[-1] == ''):
        del path[-1]
    return reduce(lambda d, key: d[key], path, paramTree)

class OdinDataController(object):
    def __init__(self, name, endpoints, update_interval=0.5):
        self._clients = []
        self._client_connections = []
        self._update_interval = update_interval
        self._name = name
        self._api = 0.1
        self._error = ""
        self.config_metadata_hash = -1
        self.status_metadata_hash = -1
        self.config_metadata_hash_prev = 0
        self.status_metadata_hash_prev = 0
        self._endpoints = []
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
                if(resp_type == IpcTornadoClient.IPC_VAL_CONFIG):
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
        path = []
        params = self.recursive_splice(index, resp_type, path, params, metadata)
        return params

    def _update_params_with_metadata(self, value_dict:dict, index:int, value_key:str, param_key:str, metadata_key:str, metadata_hash_key:str):
        # NB: 'STATUS' can be replace with 'CONFIG' in the following comments:
        #   IpcTornadoClient.STATUS_PARAMS_KEY == param_key
        #   client.parameters[IpcTornadoClient.IPC_VAL_STATUS] == value_dict
        #   IpcTornadoClient.IPC_VAL_STATUS == value_key
        #   IpcTornadoClient.IPC_VAL_STATUS_METADATA_HASH == metadata_hash_key
        #   IpcTornadoClient.IPC_VAL_STATUS_METADATA == metadata_key
        resp = None
        hash_val = 0
        if(param_key in value_dict):
            resp = value_dict[param_key]
            if(value_key == IpcTornadoClient.IPC_VAL_CONFIG):
                self._config_resposes[index] = resp
            elif(value_key == IpcTornadoClient.IPC_VAL_STATUS):
                self._status_resposes[index] = resp
        if(metadata_hash_key in value_dict):
            hash_val = value_dict[metadata_hash_key]
        if metadata_key in value_dict:
            metadata = value_dict[metadata_key]
            if(resp is not None):
                resp = self.splice_params_metadata(index, value_key, resp, metadata)
                self._params.replace(f"{index}/{value_key}", resp)
        return hash_val

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
                            # Check if the previous values of the config and status metadata hash matches the latest value.
                            # If they do not match, set with_metadata to True and update the previous hash value with the latest.
                            if(param_req == IpcTornadoClient.IPC_VAL_REQ_CFG and self.config_metadata_hash != self.config_metadata_hash_prev):
                                with_metadata = True
                                self.config_metadata_hash_prev = self.config_metadata_hash
                            elif(param_req == IpcTornadoClient.IPC_VAL_STATUS and self.status_metadata_hash != self.status_metadata_hash_prev):
                                with_metadata = True
                                self.status_metadata_hash_prev = self.status_metadata_hash
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
                    # Always track/update the hash value of the config and status metadata values
                    # using the variables status_metadata_hash & config_metadata_hash variables
                    if IpcTornadoClient.IPC_VAL_STATUS in client.parameters and \
                        client.parameters[IpcTornadoClient.IPC_VAL_STATUS][IpcTornadoClient.STATUS_PARAMS_KEY][IpcTornadoClient.CLIENT_CONNECTED]:
                        self.status_metadata_hash = self._update_params_with_metadata(client.parameters[IpcTornadoClient.IPC_VAL_STATUS],
                                                                                        index, IpcTornadoClient.IPC_VAL_STATUS,
                                                                                        IpcTornadoClient.STATUS_PARAMS_KEY,
                                                                                        IpcTornadoClient.IPC_VAL_STATUS_METADATA,
                                                                                        IpcTornadoClient.IPC_VAL_STATUS_METADATA_HASH)
                    if IpcTornadoClient.IPC_VAL_CONFIG in client.parameters:
                        self.config_metadata_hash = self._update_params_with_metadata(client.parameters[IpcTornadoClient.IPC_VAL_CONFIG],
                                                                                        index, IpcTornadoClient.IPC_VAL_CONFIG,
                                                                                        IpcTornadoClient.CONFIG_PARAMS_KEY,
                                                                                        IpcTornadoClient.IPC_VAL_CONFIG_METADATA,
                                                                                        IpcTornadoClient.IPC_VAL_CONFIG_METADATA_HASH)
                    if "commands" in client.parameters:
                        self.parse_available_commands(index, client)
                self.process_updates()
            except Exception as ex:
                logging.error("{}".format(ex))

            time.sleep(self._update_interval)

    def parse_available_commands(self, index, client):
        # Check for differences in the command structure
        # If differences exist build a new ParameterTree structure for commands
        diff = DeepDiff(self._supported_commands[index], client.parameters["commands"])
        if diff:
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
