import logging
from threading import RLock

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException


class OdinDataAdapterClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    def __init__(self, ip_address, port):
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port
        self._client_callbacks = []

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        self.ctrl_channel.register_callback(self._callback)

        self._lock = RLock()


    def _callback(self, msg):
        # Handle the multi-part message
        self.logger.debug("Msg received: %s", msg)
        reply = IpcMessage(from_str=msg[0])
        callbacks = self._client_callbacks.pop()
        self.logger.debug("Responding to callback with ID: %s", callbacks)
        for index in callbacks:
            if callbacks[index]:
                callbacks[index](index, reply.attrs)

    def _send_message(self, msg, msg_ID=None, callback=None):
        self.logger.debug("Sending control message:\n%s", msg.encode())
        with self._lock:
            self._client_callbacks.append({msg_ID: callback})
            self.ctrl_channel.send(msg.encode())

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got no response." % msg)

    def send_request(self, value, msg_ID=None, callback=None):
        msg = IpcMessage("cmd", value)
        self._send_message(msg, msg_ID, callback)

    def send_configuration(self, content, target=None, valid_error=None, msg_ID=None, callback=None):
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        self._send_message(msg, msg_ID, callback)


