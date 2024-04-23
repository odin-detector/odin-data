"""
Created on 30th November 2023

:author: Alan Greer
"""

import logging
import threading
import time

from deepdiff import DeepDiff
from odin.adapters.parameter_tree import ParameterTree

from odin_data.control.ipc_tornado_client import IpcTornadoClient


class OdinDataController(object):
    def __init__(self, name, endpoints, update_interval=0.5):
        self._clients = []
        self._client_connections = []
        self._update_interval = update_interval
        self._name = name
        self._api = 0.1
        self._error = ""
        self._endpoints = []
        self._config_cache = None
        self._first_update = False

        for arg in endpoints.split(","):
            arg = arg.strip()
            logging.debug("Endpoint: %s", arg)
            ep = {"ip_address": arg.split(":")[0], "port": int(arg.split(":")[1])}
            self._endpoints.append(ep)

        for ep in self._endpoints:
            logging.debug("Creating client {}:{}".format(ep["ip_address"], ep["port"]))
            self._clients.append(IpcTornadoClient(ep["ip_address"], ep["port"]))
            self._client_connections.append(False)

        self._params = ParameterTree(
            {
                "api": (lambda: self._api, None, {}),
                "module": (lambda: self._name, None, {}),
                "endpoints": (lambda: self._endpoints, None, {}),
                "count": (lambda: len(self._clients), None, {}),
                "update_interval": (lambda: self._update_interval, None, {}),
                "status": {"error": (lambda: self._error, None, {})},
                "config": {},
            },
            mutable=True,
        )

        # Create the status loop handling thread
        self._status_running = True
        self._status_lock = threading.Lock()
        self._status_thread = threading.Thread(target=self.update_loop)
        self._status_thread.start()

    @property
    def first_update(self):
        return self._first_update

    def set_error(self, err):
        # Record the error message into the status
        self._error

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
        return self._params.get(path, meta)

    def put(self, path, value):
        self._params.set(path, value)
        self.process_config_changes()

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
                status_tree = [{}] * len(self._clients)
                config_tree = [{}] * len(self._clients)
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
                                # Reconnection event so push configuration
                                logging.debug("Client reconnection event")
                    #                            self.process_reconnection(index)

                    except Exception as e:
                        # Exception caught, log the error but do not stop the update loop
                        logging.error("Unhandled exception: %s", e)

                    # Request parameter updates
                    for parameter_tree in ["status", "request_configuration"]:
                        try:
                            msg = client.send_request(parameter_tree)
                            if client.wait_for_response(msg.get_msg_id()):
                                # timed_out = True
                                pass
                        except Exception as e:
                            # Log the error, but do not stop the update loop
                            logging.error("Unhandled exception: %s", e)

                    if "status" in client.parameters:
                        status_tree[index] = client.parameters["status"]
                    if "config" in client.parameters:
                        config_tree[index] = client.parameters["config"]

                self._params.replace("status", status_tree)
                self._params.replace("config", config_tree)
                self._config_cache = self._params.get("config")
                # Flag that we have made an update
                self._first_update = True

                self.process_updates()

            except Exception as ex:
                logging.error("{}".format(ex))

            time.sleep(self._update_interval)

    def process_config_changes(self):
        """Search through the application config trees and compare with the
        latest cached version.  Any changes should be built into a new config
        message and sent down to the applications.
        This method must be called after the set method is called and needs to
        be executed in its own thread to avoid blocking the tornado loop.
        """
        new_config = self._params.get("config")
        diff = DeepDiff(self._config_cache, new_config)

        if "values_changed" in diff:
            logging.debug("Config deltas: %s", diff)
            # Build an array of configurations from any differences.
            # There will be 1 configuration object for each client (if differences
            # are present)
            configs = [None] * len(self._clients)
            for root in diff["values_changed"]:
                # Clean up the root string removing start and end constants
                path = (
                    root.replace("root[", "").rstrip("]").replace("'", "").split("][")
                )
                logging.error("Path: {}".format(path))
                # First element of the path is the key 'config'
                # Second element of the path is the index of the client
                index = int(path[1])
                # Build the config for this root
                if configs[index] is None:
                    configs[index] = {}
                client_cfg = configs[index]
                logging.debug("Client config [%s]: %s", index, client_cfg)
                cfg = client_cfg
                for item in path[2:-1]:
                    if item not in cfg:
                        cfg[item] = {}
                    cfg = cfg[item]
                cfg[path[-1]] = diff["values_changed"][root]["new_value"]
                # configs[index] = {**configs[index], **client_cfg}

            logging.error("Sending configs: %s", configs)

            # Loop through the new params
            index = 0
            for config in configs:
                if config is not None:
                    self._clients[index].send_configuration(config)
                index += 1

    def create_demand_config(self, new_params, old_params):
        config = None
        for item in new_params:
            logging.error("Param: {}".format(item))
            logging.error("   Type: {}".format(type(new_params[item])))
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
                    #                    logging.error("List: {}".format(new_params[item]))
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
