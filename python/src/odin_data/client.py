"""Simple command-line client for interacting with and debugging odin-data applications.

This module implements a simple command-line client that can be used to interact with
and debug odin-data applications via their control interface.

Tim Nicholls, STFC Detector Systems Software Group
"""
import argparse
import json
import logging
import os
import sys

from odin_data.control.ipc_channel import IpcChannel
from odin_data.control.ipc_message import IpcMessage

try:
    from json.decoder import JSONDecodeError
except ImportError:
    JSONDecodeError = ValueError


class OdinDataClient(object):
    """odin-data client.

    This class implements the odin-data command-line client.
    """

    def __init__(self):
        """Initialise the odin-data client."""
        prog_name = os.path.basename(sys.argv[0])

        # Parse command line arguments
        self.args = self._parse_arguments(prog_name)

        # create logger
        self.logger = logging.getLogger(prog_name)
        self.logger.setLevel(logging.DEBUG)

        # create console handler and set level to debug
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(self.args.log_level)

        # create formatter
        formatter = logging.Formatter(
            "%(asctime)s %(levelname)s %(name)s - %(message)s"
        )

        # add formatter to ch
        ch.setFormatter(formatter)

        # add ch to logger
        self.logger.addHandler(ch)

        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.args.ctrl_endpoint)

        self._msg_id = 0
        self._run = True

    def _parse_arguments(self, prog_name=sys.argv[0]):
        """Parse arguments from the command line."""
        parser = argparse.ArgumentParser(
            prog=prog_name, description="odin-data application client"
        )
        parser.add_argument(
            "--ctrl",
            type=str,
            default="tcp://127.0.0.1:5000",
            dest="ctrl_endpoint",
            help="Specify the IPC control channel endpoint URL",
        )
        parser.add_argument(
            "--config",
            type=argparse.FileType("r"),
            dest="config_file",
            nargs="?",
            default=None,
            const=sys.stdin,
            help="Specify JSON configuration file to send as configure command",
        )
        parser.add_argument(
            "--request-config",
            action="store_true",
            help="Request configuration from odin-data application",
        )
        parser.add_argument(
            "--status",
            action="store_true",
            help="Request a status report from the odin-data application",
        )
        parser.add_argument(
            "--shutdown",
            action="store_true",
            help="Instruct the odin-data application to shut down",
        )
        # parser.add_argument(
        #     "params",
        #     default="",
        #     help="Filter tree of parameters in reply",
        # )
        parser.add_argument(
            "--log-level",
            choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
            default="CRITICAL",
            help="Set the logging level",
        )

        args = parser.parse_args()
        return args

    def _next_msg_id(self):
        """Return the next IPC message ID to use."""
        self._msg_id += 1
        return self._msg_id

    def run(self):
        """Run the odin-data client."""
        self.logger.info("odin-data client starting up")

        self.logger.debug(
            "Control IPC channel has identity {}".format(self.ctrl_channel.identity)
        )

        if self.args.config_file is not None:
            reply = self.do_config_cmd(self.args.config_file)

        if self.args.request_config:
            reply = self.do_request_config_cmd()

        if self.args.status:
            reply = self.do_status_cmd()

        if self.args.shutdown:
            reply = self.do_shutdown_cmd()

        print(reply)

    def do_config_cmd(self, config_file):
        """Send a configure command populated from a JSON configuration file"""
        try:
            config_params = json.load(config_file)

            config_msg = IpcMessage("cmd", "configure", id=self._next_msg_id())
            for param, value in config_params.items():
                config_msg.set_param(param, value)

            self.logger.info(
                "Sending configure command to the odin-data application"
                " with specified parameters"
            )
            self.ctrl_channel.send(config_msg.encode())
            self.await_response()

        except JSONDecodeError as e:
            self.logger.error("Failed to parse configuration file: {}".format(e))

    def do_status_cmd(self):
        """Send a status command to odin-data."""
        status_msg = IpcMessage("cmd", "status", id=self._next_msg_id())
        self.logger.info("Sending status request to the odin-data application")
        self.ctrl_channel.send(status_msg.encode())
        return self.await_response()

    def do_request_config_cmd(self):
        """Send a request configurtion command to odin-data."""
        status_msg = IpcMessage("cmd", "request_configuration", id=self._next_msg_id())
        self.logger.info("Sending configuration request to the odin-data application")
        self.ctrl_channel.send(status_msg.encode())
        return self.await_response()

    def do_shutdown_cmd(self):
        """Send a shutdown command to odin-data."""
        shutdown_msg = IpcMessage("cmd", "shutdown", id=self._next_msg_id())
        self.logger.info("Sending shutdown command to the odin-data application")
        self.ctrl_channel.send(shutdown_msg.encode())
        return self.await_response()

    # def _send_request(self, msg_val: str) -> str:

    def await_response(self, timeout_ms=1000):
        """Await a response to a client command."""
        pollevts = self.ctrl_channel.poll(timeout_ms)
        if pollevts == IpcChannel.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            self.logger.info("Got response: {}".format(reply))

        return reply


def main():
    """Run the odin-data client."""
    app = OdinDataClient()
    app.run()


if __name__ == "__main__":
    main()
