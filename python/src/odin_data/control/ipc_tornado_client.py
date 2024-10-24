import logging
from threading import RLock
from time import sleep

from odin_data.control.ipc_message import IpcMessage, IpcMessageException
from odin_data.control.ipc_tornado_channel import IpcTornadoChannel


class IpcTornadoClient(object):
    """Client class that wraps an IpcTornadoClient class with additional
    message handling and management.

    """
    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"
    MSG_CHECK_SLEEP_DELTA = 0.01
    IPC_VAL_REQ_VERS = "request_version"
    IPC_VAL_REQ_CFG = "request_configuration"
    IPC_VAL_REQ_CMDS = "request_commands"
    IPC_VAL_STATUS = "status"
    CLIENT_CONNECTED = "connected"

    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip_address, port):
        """Initalise the IpcTornadoClient object.

        This creates a DEALER ZeroMQ socket for communication with the following
        applications:
        - FrameReceiver
        - FrameProcessor
        - MetaWriter

        :param ip_address: IP address for the ZeroMQ socket endpoint
        :param port: UPort number for the ZeroMQ socket endpoint
        """
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port

        self._parameters = {self.IPC_VAL_STATUS: {self.CLIENT_CONNECTED: False}}
        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.register_monitor(self._monitor_callback)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        self.ctrl_channel.register_callback(self._callback)
        self.message_id = 0

        # Dictionary of outstanding config messages
        self._active_msgs = {}
        self._rejected_configs = {}

        self._lock = RLock()

    @property
    def parameters(self):
        """Read out the stored parameters from this client.

        The parameters dictionary contains a snapshot of all responses to
        messages sent out by this client.  The latest resposnes are
        stored in the _parameters dict and are returned by this method.

        :return: the _parameters dict
        """
        return self._parameters

    def read_rejected_configs(self):
        """Return the current dict of rejected configuration messages.

        :return: a dict of rejected configuration
        """
        return self._rejected_configs

    def clear_rejected_configs(self):
        """Clear any rejected configuration messages.
        """
        self.logger.debug("Clearing rejected configurations")
        with self._lock:
            self._rejected_configs.clear()

    def _monitor_callback(self, msg):
        """Handles incoming messages from the monitoring ZeroMQ socket.

        The monitoring socket receives notification messages when the
        main socket state changes (eg a connection is established).
        When a state change occurs this method is called with the
        notification.

        The notification is checked for a connected or disconnected
        event and the connected status parameter is updated to reflect
        this new state of the socket.

        :param msg: Incoming message from the monitoring ZeroMQ socket
        """
        self.logger.debug("Msg received from %s: %s", self.ctrl_endpoint, msg)
        if msg['event'] == IpcTornadoChannel.CONNECTED:
            self.logger.debug("  Connected...")
            self._parameters[self.IPC_VAL_STATUS][self.CLIENT_CONNECTED] = True
        if msg['event'] == IpcTornadoChannel.DISCONNECTED:
            self.logger.debug("  Disconnected...")
            self._parameters[self.IPC_VAL_STATUS][self.CLIENT_CONNECTED] = False

    def _callback(self, msg):
        """Handes incoming messages from the main ZeroMQ socket.

        The handled message types are:
        - Reply to 'request_version'
        - Reply to 'request_configuration'
        - Reply to 'request_commands'
        - Reply to 'status'
        - Reply to 'configure'

        Replies to request_version, request_configuration and status
        are stored in the _parameters according to the message type.
        Replies to 'configure' result in the corresponding message
        being removed from the _active_msgs dict.  They are then
        checked for rejection and moved into the _rejected_configs
        dict if they were rejected.

        :param msg: Incoming message from the main ZeroMQ socket
        """
        # Handle the multi-part message
        reply = IpcMessage(from_str=msg[0])
        msg_val = reply.get_msg_val()
        if self.IPC_VAL_REQ_VERS in msg_val:
            self._update_versions(reply.attrs)
        elif self.IPC_VAL_REQ_CFG in msg_val:
            self._update_configuration(reply.attrs)
        elif self.IPC_VAL_STATUS in msg_val:
            self._update_status(reply.attrs)
        elif self.IPC_VAL_REQ_CMDS in msg_val:
            self._update_commands(reply.attrs)
        with self._lock:
            self._active_msgs.pop(reply.get_msg_id())
            if reply.get_msg_type() == "nack" and reply.get_msg_val() == "configure":
                with self._lock:
                    self._rejected_configs[reply.get_msg_id()] = reply

    def _update_versions(self, version_msg):
        """Store the response to a request_version message in the
        _parameters dict.

        :param version_msg: Incoming version message response
        """
        params = version_msg['params']
        self._parameters['version'] = params['version']

    def _update_configuration(self, config_msg):
        """Store the response to a request_configuration message in the
        _parameters dict.

        :param config_msg: Incoming configuration message response
        """
        params = config_msg['params']
        self._parameters['config'] = params

    def _update_status(self, status_msg):
        """Store the response to a status message in the _parameters dict.

        :param status_msg: Incoming status message response
        """
        params = status_msg['params']
        params['timestamp'] = status_msg['timestamp']
        self._parameters[self.IPC_VAL_STATUS] = params
        if 'error' not in self._parameters[self.IPC_VAL_STATUS]:
            self._parameters[self.IPC_VAL_STATUS]['error'] = []
        with self._lock:
            for msg in self._rejected_configs:
                self._parameters[self.IPC_VAL_STATUS]['error'].append(self._rejected_configs[msg].get_params()['error'])

        # If we have received a status response then we must be connected
        self._parameters[self.IPC_VAL_STATUS][self.CLIENT_CONNECTED] = True

    def _update_commands(self, commands_msg):
        """Store the response to a request_commands message in the _parameters dict.

        :param commands_msg: Incoming commands message reponse
        """
        params = commands_msg['params']
        self._parameters['commands'] = params

    def connected(self):
        """Returns the connected state of this client.

        :return: The connected state of the client
        """
        return self._parameters[self.IPC_VAL_STATUS][self.CLIENT_CONNECTED]

    def _send_message(self, msg):
        """Injects the next message ID into the message object.
        Adds the message to the list of active messages (indexed by
        unique ID).  Sends the encoded ASCII message through the
        ZeroMQ socket.

        :param msg: The message to send
        """
        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        self.logger.debug("Sending control message [%s]:\n%s", self.ctrl_endpoint, msg.encode())
        with self._lock:
            self._active_msgs[msg.get_msg_id()] = msg
            self.ctrl_channel.send(msg.encode())

        return msg

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got no response." % msg)

    def send_request(self, value):
        """Creates an IpcMessage object with the specified value
        and calls the _send_message method with the message object.

        :param value: Value of the IpcMessage object
        """
        msg = IpcMessage("cmd", value)
        return self._send_message(msg)

    def send_configuration(self, content, target=None, valid_error=None):
        """Creates a specific configure IpcMessage object.  Sets the
        parameters of the configure message object to content and sends
        the message

        :param content: contents of message to send
        :return: The message object that has been created and sent
        """
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        if "clear_errors" in msg.get_params():
            self.clear_rejected_configs()

        return self._send_message(msg)

    def execute_command(self, plugin, command):
        """

        :param plugin: the name of the plugin to which the command belongs
        :param command: the name of the command to execute
        :return: The message object that has been created and sent

        """
        msg = IpcMessage("cmd", "execute")
        msg.set_param(plugin, {"command": command})

        return self._send_message(msg)

    def check_for_rejection(self, msg_id):
        """Check specific message for rejection.

        :param msg_id: the ID of the message to search for
        :return: boolean True if the config has been rejected
        """
        rejected = False
        if msg_id in self._rejected_configs:
            rejected = True
        return rejected

    def wait_for_response(self, msg_id, timeout=1.0):
        """Block waiting for a message response, with a timeout

        Check to see if message is in the active message dict.
        If it is, wait for sleep delta. Keep checking until a
        timeout occurs or a response arrives.

        :param msg_id: the ID of the message to wait for
        :param timeout: number of seconds to block for before timing out.  If
        timeout is set to less than zero then block forever
        :return: a boolean True if a timout occured
        """
        timed_out = False
        running = True
        sleep_time = 0.0
        while running:
            with self._lock:
                if msg_id not in self._active_msgs:
                    running = False
            if running:
                if timeout >= 0.0:
                    sleep(self.MSG_CHECK_SLEEP_DELTA)
                    sleep_time += self.MSG_CHECK_SLEEP_DELTA
                    if sleep_time >= timeout:
                        running = False
                        timed_out = True

        return timed_out
