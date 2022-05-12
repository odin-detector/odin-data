"""Implementation of odin_data Meta Listener

This module listens on one or more ZeroMQ sockets for meta messages which it then passes
on to a MetaWriter to write. Also handles odin Ipc control messages.

Matt Taylor, Diamond Light Source
"""
import logging
import re
from json import loads
import importlib

import zmq
from zmq.utils.strtypes import cast_bytes

from odin_data.ipc_message import IpcMessage
import odin_data._version as versioneer
from odin_data.util import construct_version_dict

DEFAULT_ACQUISITION_ID = ""
DEFAULT_DIRECTORY = "/tmp"

WRITER = "writer"
ENDPOINT = "_endpoint"


class MetaListener(object):
    """
    This class listens on ZeroMQ sockets for incoming control messages for
    configuration and gathers data messages from data endpoints to pass on to
    the loaded writer class.

    """

    def __init__(self, ctrl_port, data_endpoints, writer, writer_config):
        """
        Args:
            ctrl_port(int): The port to bind the control channel to
            data_endpoints(list(str)): List of endpoints to receive data on
                e.g. ["tcp://127.0.0.1:5008", "tcp://127.0.0.1:5018"]
            writer(str): Full import path for writer class to load
            writer_config(MetaWriterConfig): Configuration to pass to writers

        """
        self._ctrl_port = ctrl_port
        self._data_endpoints = data_endpoints
        self._writers = {}
        self._status_dict = {}
        self._writer_names = []
        self._shutdown_requested = False

        self._logger = logging.getLogger(self.__class__.__name__)

        self._writer_class = self._load_writer(writer)
        self._writer_config = writer_config

    @staticmethod
    def _construct_reply(msg_val, msg_id, error=None):
        reply = IpcMessage(IpcMessage.ACK, msg_val, id=msg_id)

        if error is not None:
            reply.set_msg_type(IpcMessage.NACK)
            reply.set_param("error", error)

        return reply

    def run(self):
        """Main application loop"""
        context = zmq.Context()
        ctrl_endpoint = "tcp://*:{}".format(self._ctrl_port)
        self._logger.info("Binding control address to %s", ctrl_endpoint)
        ctrl_socket = context.socket(zmq.ROUTER)
        ctrl_socket.bind(ctrl_endpoint)

        data_sockets = {}
        for endpoint in self._data_endpoints:
            socket = context.socket(zmq.SUB)
            socket.set_hwm(10000)
            socket.connect(endpoint)
            socket.setsockopt(zmq.SUBSCRIBE, cast_bytes(""))
            data_sockets[endpoint] = socket


        poller = zmq.Poller()
        poller.register(ctrl_socket, zmq.POLLIN)
        for socket in data_sockets.values():
            poller.register(socket, zmq.POLLIN)

        self._logger.info(
            "Listening for data messages on: %s", ", ".join(self._data_endpoints)
        )
        try:
            while not self._shutdown_requested:
                sockets = dict(poller.poll())

                if sockets.get(ctrl_socket) == zmq.POLLIN:
                    self.handle_control_message(ctrl_socket)

                # Delay handling data until we have a writer configured
                if not self._writers:
                    continue

                for endpoint, socket in data_sockets.items():
                    if sockets.get(socket) == zmq.POLLIN:
                        self.handle_data_message(socket, endpoint)
        except KeyboardInterrupt:
            self._logger.info("Received KeyboardInterrupt")
        except:
            self._logger.error("Unexpected exception - shutting down")
            raise
        else:
            self._logger.info("Shutdown requested")
        finally:
            try:
                self.stop_all_writers()
            except:
                self._logger.error("Exception when stopping writers to shutdown")

            self._logger.info("Closing sockets")
            for socket in data_sockets.values():
                socket.close(linger=0)
            if ctrl_socket is not None:
                ctrl_socket.close(linger=100)
            context.term()

        self._logger.info("Finished")

    def handle_data_message(self, socket, endpoint):
        """Handle a data message on the given socket

        Args:
            socket(zmq.Socket): The socket to receive a message on

        """
        self._logger.debug("Handling data message from {}".format(endpoint))

        # Receive the first part of the two part message
        # This is a json string with the following:
        #   - header: Any control parameters that do not go into the data file
        #       e.g. the total number of frames that will be written
        #   - type: The type of the second part of the message
        #   - parameter: The message type ID to determine how to handle it
        #   - plugin: The source, e.g. the class name that sent the message

        header = socket.recv_json()
        self._logger.debug("Header message:\n%s", header)

        # Messages are filed according to their starting location.  The endpoint
        # is unique to each FP application and so this source endpoint of the 
        # message is stored in the header so that it can be used within the writer.
        header["header"][ENDPOINT] = endpoint

        acquisition_id = header["header"].get("acqID", DEFAULT_ACQUISITION_ID)
        if acquisition_id == DEFAULT_ACQUISITION_ID:
            self._logger.debug("Using default acquisition ID")
        if acquisition_id not in self._writers:
            self._logger.debug(
                "No writer configured for acquisition ID %s", acquisition_id
            )
            socket.recv()
            return

        writer = self._writers[acquisition_id]
        if writer.finished:
            self._logger.debug(
                "Writer for acquisition ID %s already finished", acquisition_id
            )
            socket.recv()
            return

        # Receive the second part of the two part message
        # This contains the actual meta data to be written to file
        # The content could be:
        #   - A json string with one or more meta data items
        #   - A data blob - for example a pixel mask

        if header["type"] == "raw":
            data = socket.recv()
            self._logger.debug("Data message part is a data blob")
        else:
            data = socket.recv()
            if data:
                try:
                    data = loads(data)
                except ValueError:
                    self._logger.error("Failed to load json from message: %s", data)
                    return
                else:
                    self._logger.debug("Data message:\n%s", data)
            else:
                self._logger.debug("Data message empty")

        writer.process_message(header, data)

    # Methods for handling the control channel

    def handle_control_message(self, socket):
        """Handle a control message on the given socket

        Args:
            socket(zmq.Socket): The socket to receive a message and reply on

        """
        message_handlers = {
            "status": self.status,
            "configure": self.configure,
            "request_configuration": self.request_configuration,
            "request_version": self.version,
            "shutdown": self.shutdown,
        }

        # The first message part is a channel ID
        channel_id = socket.recv()

        # The second message part is the IpcMessage
        message = IpcMessage(from_str=socket.recv())
        request_type = message.get_msg_val()

        handler = message_handlers.get(request_type, None)
        if handler is not None:
            reply = handler(message)
        else:
            error = "Unknown request type: {}".format(request_type)
            self._logger.error(error)
            reply = self._construct_reply(
                message.get_msg_val(), message.get_msg_id(), error
            )

        socket.send(channel_id, zmq.SNDMORE)
        socket.send(cast_bytes(reply.encode()))

    def status(self, request):
        """Handle a status request message

        Args:
            request(IpcMessage): The request message

        Returns:
            reply(IpcMessage): Reply containing current status

        """
        self._logger.debug("Handling status request")

        # Clear status from old writers
        status_dict = {}
        for writer_name in self._writer_names:
            if writer_name in self._status_dict:
                status_dict[writer_name] = self._status_dict[writer_name]
        self._status_dict = status_dict

        for writer_name in self._writers:
            writer = self._writers[writer_name]
            self._status_dict[writer_name] = writer.status()
            writer.write_timeout_count = writer.write_timeout_count + 1

        reply = self._construct_reply(request.get_msg_val(), request.get_msg_id())
        reply.set_param("acquisitions", self._status_dict)
        self._logger.debug("Status dict: {}".format(self._status_dict))

        if len(self._writers) > 1:
            self.clear_writers()

        return reply

    def clear_writers(self):
        unfinished_writers = {}
        for writer_name, writer in self._writers.items():
            self._status_dict[writer_name] = writer.status()
            if writer.finished:
                self._logger.debug("Deleting writer: {}".format(writer_name))
            else:
                # TODO: This is bit of a hack...
                stagnant = (
                    writer.active_process_count == 0
                    and writer.write_timeout_count > 10
                    and writer.file_open
                )
                if stagnant:
                    self._logger.info("Stopping stagnant writer %s", writer_name)
                    writer.stop()
                unfinished_writers[writer_name] = writer
        self._writers = unfinished_writers

    def configure(self, request):
        """Handle a configuration message

        Args:
            request(IpcMessage): The request message

        Returns:
            reply(IpcMessage): Reply containing current configuration

        """
        params = request.get_params()
        self._logger.debug("Handling configure:\n%s", params)

        error = None
        if "acquisition_id" in params:
            # Read and remove acquisition ID from params - writer does not need it
            writer_name = params.pop("acquisition_id")

            # Check for stop before anything else
            if "stop" in params:
                if writer_name is not None and writer_name in self._writers:
                    self._logger.info("Received stop for writer %s", writer_name)
                    self._writers[writer_name].stop()
                else:
                    # Stop without an acquisition ID stops all acquisitions
                    self.stop_all_writers()
            else:
                if (
                    writer_name in self._writers
                    and not self._writers[writer_name].finished
                ):
                    self._logger.debug("Configuring existing writer: %s", writer_name)
                else:
                    self.create_new_writer(writer_name)

                self._logger.info(
                    "Configuring writer %s with params: %s", writer_name, params
                )
                error = self._writers[writer_name].configure(params)

        else:
            error = "No params in config"

        return self._construct_reply(request.get_msg_val(), request.get_msg_id(), error)

    def create_new_writer(self, writer_name):
        """Create a new writer to handle meta messages for a new acquisition

        Args:
            writer_name(str): Unique name to identify this writer

        """
        # First finish any writer that is queued up already but has not started
        for _writer_name, writer in self._writers.items():
            if not writer.file_open and not writer.finished:
                self._logger.info("Stopping idle writer: %s", _writer_name)
                writer.stop()

        # Now create new acquisition
        self._logger.info("Creating new writer %s", writer_name)
        self._writers[writer_name] = self._writer_class(
            writer_name, DEFAULT_DIRECTORY, self._data_endpoints, self._writer_config
        )
        # Register the writer name but only keep a record of the last three
        while len(self._writer_names) > 2:
            self._writer_names.pop(0)
        self._writer_names.append(writer_name)
        self._logger.debug("Registered writer name list: {}".format(self._writer_names))

        # Check if we have too many writers and delete any that are finished
        if len(self._writers) > 3:
            self.clear_writers()

    def stop_all_writers(self):
        """Iterate all writers and call stop"""
        self._logger.info("Stopping all writers")
        for writer in self._writers.values():
            writer.stop()
        self.clear_writers()

    def _load_writer(self, writer):
        """Import the writer class from the configured module

        Returns:
            class: The writer class to instantiate

        """
        self._logger.debug("Loading writer class: %s", writer)

        module, class_name = writer.rsplit(".", 1)
        module = importlib.import_module(module)
        writer_class = getattr(module, class_name)

        return writer_class

    def request_configuration(self, request):
        """Handle a configuration request message

        Args:
            request(IpcMessage): The request message

        Returns:
            reply(IpcMessage): Reply containing current configuration

        """
        self._logger.debug("Handling request configuration")

        writer_config = dict(
            (writer_name, writer.request_configuration())
            for writer_name, writer in self._writers.items()
        )

        reply = self._construct_reply(request.get_msg_val(), request.get_msg_id())
        reply.set_param("acquisitions", writer_config)
        reply.set_param("data_endpoints", self._data_endpoints)
        reply.set_param("ctrl_port", self._ctrl_port)
        return reply

    def version(self, request):
        """Handle request version message

        Args:
            request(IpcMessage): The request message

        Returns:
            reply(IpcMessage): Reply containing version status

        """
        self._logger.debug("Handling version request")

        version = versioneer.get_versions()["version"]

        writer_repo, writer_version_dict = self._writer_class.get_version()
        version_dict = {
            "odin-data": construct_version_dict(version),
            writer_repo: writer_version_dict,
        }

        reply = self._construct_reply(request.get_msg_val(), request.get_msg_id())
        reply.set_param("version", version_dict)
        return reply

    def shutdown(self, request):
        """Handle request shutdown message

        Args:
            request(IpcMessage): The request message

        Returns:
            reply(IpcMessage): Reply confirming shutdown

        """
        self._logger.info("Handling shutdown request")
        self._shutdown_requested = True
        return self._construct_reply(request.get_msg_val(), request.get_msg_id())
