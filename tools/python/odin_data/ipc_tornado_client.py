import logging
import struct
from threading import RLock

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException
from datetime import datetime


class IpcTornadoClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip_address, port):
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port

        self._parameters = {'status': {'connected': False}}
        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.register_monitor(self._monitor_callback)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        self.ctrl_channel.register_callback(self._callback)
        self.message_id = 0

        self._lock = RLock()

    @property
    def parameters(self):
        return self._parameters

    def _monitor_callback(self, msg):
        # Handle the multi-part message
        self.logger.debug("Msg received from %s: %s", self.ctrl_endpoint, msg)
        if msg['event'] == IpcTornadoChannel.CONNECTED:
            self.logger.debug("  Connected...")
            self._parameters['status']['connected'] = True
        if msg['event'] == IpcTornadoChannel.DISCONNECTED:
            self.logger.debug("  Disconnected...")
            self._parameters['status']['connected'] = False

    def _callback(self, msg):
        # Handle the multi-part message
        reply = IpcMessage(from_str=msg[0])
        if 'request_version' in reply.get_msg_val():
            self._update_versions(reply.attrs)
        if 'request_configuration' in reply.get_msg_val():
            self._update_configuration(reply.attrs)
        if 'status' in reply.get_msg_val():
            self._update_status(reply.attrs)

    def _update_versions(self, version_msg):
        params = version_msg['params']
        self._parameters['version'] = params['version']

    def _update_configuration(self, config_msg):
        params = config_msg['params']
        self._parameters['config'] = params

    def _update_status(self, status_msg):
        params = status_msg['params']
        params['timestamp'] = status_msg['timestamp']
        self._parameters['status'] = params
        # If we have received a status response then we must be connected
        self._parameters['status']['connected'] = True

    def connected(self):
        return self._parameters['status']['connected']

    def _send_message(self, msg):
        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        self.logger.debug("Sending control message [%s]:\n%s", self.ctrl_endpoint, msg.encode())
        with self._lock:
            self.ctrl_channel.send(msg.encode())

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got no response." % msg)

    def send_request(self, value):
        msg = IpcMessage("cmd", value)
        self._send_message(msg)

    def send_configuration(self, content, target=None, valid_error=None):
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        self._send_message(msg)


