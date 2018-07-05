"""Monitoring of process statistics running on a server.

This module provides a mechanism for monitoring processes running on a server.
It provides a standard odin-data IPC control and status interface and so can be
included into an odin control server (via appropriate adapter).

Created on 20 June 2018

Alan Greer, OSL
"""
from __future__ import print_function

import argparse
import logging
import os
import psutil
from ipc_channel import IpcChannel
from ipc_message import IpcMessage
from ipc_reactor import IpcReactor


class ServerMonitor(object):
    """Class to monitor processes running on a server.

    This class maintains a list of processes running on a machine and periodically
    updates statistics relating to the processes.  Currently the statistics include:
    cpu percent usage
    cpu affinity
    memory usage
    memory percent usage

    This class provides a standard Odin IPC control interface for control and monitoring
    through an adapter.
    The list of processes to monitor can be set from the command line parameters or sent through
    the IPC control interface.
    """
    def __init__(self):
        """Initalise the Server Monitor.

        Creates the dictionaries for status and process monitoring.
        Creates the IPC Reactor.
        """
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        self._ctrl_channel = None
        self._reactor = IpcReactor()
        self._status = {}
        self._processes = {}

    def setup_control_channel(self, endpoint):
        """Initalise the IPC control channel.

        Creates the IPC control channel, binds it to a ZMQ socket and registers it
        with the reactor.

        :param endpoint the endpoint to setup (tcp://x.x.x.x:port)
        """
        self._ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_ROUTER)
        self._ctrl_channel.bind(endpoint)
        self._reactor.register_channel(self._ctrl_channel, self.handle_ctrl_msg)

    def setup_timer(self):
        """Set up the internal timer for monitoring statistics."""
        self._reactor.register_timer(1000, 0, self.monitor)

    def start_reactor(self):
        """Start the reactor."""
        self._log.debug("Starting reactor...")
        self._reactor.run()

    def add_process(self, process_name):
        """Add a new process to monitor.

        :param process_name the name of the process to monitor
        """
        if process_name not in self._processes:
            self._log.debug("Adding process %s to monitor list", process_name)
            self._processes[process_name] = self.find_process(process_name)

    def monitor(self):
        """Loops over active processes and mines the statistics from them.

        The values are stored into the status dictionary and made available
        through the IPC control channel
        """
        self._status = {}
        for process_name in self._processes:
            try:
                if self._processes[process_name] is None:
                    self._processes[process_name] = self.find_process(process_name)

                process = self._processes[process_name]
                if process is not None:
                    self._log.debug("Monitoring process [%s]: %s", process_name, process)
                    memory_info = process.memory_info()
                    process_data = {
                        'cpu_percent': process.cpu_percent(interval=0.0),
                        'cpu_affinity': process.cpu_affinity(),
                        'memory_percent': process.memory_percent(),
                        'memory_rss': memory_info.rss,
                        'memory_vms': memory_info.vms,
                        'memory_shared': memory_info.shared
                    }
                    self._log.debug(process_data)
                    self._status[process_name] = process_data
            except:
                # Any exception may occur due to the process not existing etc, so reset the process object
                self._processes[process_name] = None

    def find_process(self, process_name):
        """Find a process and return the process object.

        Returns a psutil process object
        """
        parent = self.find_process_by_name(process_name)
        if len(parent.children()) > 0:
            process = parent.children()[0]
        else:
            process = parent
        return process

    def find_process_by_name(self, name):
        """Return the first process matching 'name' that is not this process (in the case
        where the process name was passed as an argument to this process)."""
        proc = None
        for p in psutil.process_iter():
            if name in p.name():
                self._log.debug("Found %s in %s", name, p.name())
                proc = p
            else:
                for cmdline in p.cmdline():
                    if name in cmdline:
                        # Make sure the name isn't found as an argument to this process!
                        if os.getpid() != p.pid:
                            self._log.debug("Found %s in %s", name, p.cmdline())
                            proc = p
        if proc.status == psutil.STATUS_ZOMBIE \
            or proc.status == psutil.STATUS_STOPPED \
            or proc.status == psutil.STATUS_DEAD:
            proc = None
        return proc

    def handle_ctrl_msg(self, msg, channel_id):
        """Handle any control messages arriving on the IPC control channel

        The message type is checked to see if it is a configuration message
        or a status request and the appropriate action is taken.

        :param msg the IpcMessage object containing parameters
        :param channel_id the ID of the channel that sent the message
        ."""
        self._log.debug("Received message on configuration channel: %s", msg)
        reply_msg = IpcMessage()
        reply_msg.set_msg_val(msg.get_msg_val())
        reply_msg.set_msg_id(msg.get_msg_id())

        if msg.get_msg_type() == IpcMessage.MSG_TYPE_CMD and msg.get_msg_val() == IpcMessage.MSG_VAL_CMD_CONFIGURE:
            reply_msg.set_msg_type(IpcMessage.MSG_TYPE_ACK)
            reply_msg = self.configure(msg, reply_msg)

        elif msg.get_msg_type() == IpcMessage.MSG_TYPE_CMD and msg.get_msg_val() == IpcMessage.MSG_VAL_CMD_STATUS:
            reply_msg.set_msg_type(IpcMessage.MSG_TYPE_ACK)
            reply_msg.set_param("status", self._status)

        self._log.debug("Control thread reply message (configure): %s", reply_msg.encode())
        self._ctrl_channel.send(reply_msg.encode(), channel_id)

    def configure(self, msg, reply_msg):
        """Configure the monitor class.

        Currently this only accepts the name of a new process to monitor.

        :param msg the IpcMessage containing any configuration information
        :param reply_msg fill this in with any reply information and return it
        :returns the reply_msg"""
        if msg.has_param("process"):
            self.add_process(msg.get_param("process"))
        return reply_msg


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:7001", help="Control endpoint")
    parser.add_argument("-l", "--logconfig", default="", help="Logging configuration file")
    parser.add_argument("-p", "--process", action="append", help="Process(es) to monitor")
    args = parser.parse_args()
    return args


def main():
    args = options()
    if args.logconfig != "":
        logging.config.fileConfig(args.logconfig)

    smon = ServerMonitor()
    smon.setup_control_channel(args.control)
    smon.setup_timer()
    for process in args.process:
        smon.add_process(process)
    smon.start_reactor()


if __name__ == '__main__':
    main()
