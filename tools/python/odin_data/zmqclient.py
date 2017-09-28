import logging

import zmq
from threading import RLock
from odin_data.ipc_channel import IpcChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException


class ZMQClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    def __init__(self, ip_address, port):
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ctrl_channel.connect(self.ctrl_endpoint)

        self._lock = RLock()

    def _send_message(self, msg, timeout=1000):
        self.logger.debug("Sending control message:\n%s", msg.encode())
        with self._lock:
            self.ctrl_channel.send(msg.encode())
            pollevts = self.ctrl_channel.poll(timeout)

        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            if reply.is_valid() \
               and reply.get_msg_type() == IpcMessage.ACK:
                self.logger.debug("Request successful")
                return True, reply.attrs
            else:
                self.logger.debug("Request unsuccessful")
                return False, reply.attrs
        else:
            self.logger.warning("Received no response")
            return False, None

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException(
                "Request\n%s\nunsuccessful."
                " Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException(
                "Request\n%s\nunsuccessful."
                " Got no response." % msg)

    def send_request(self, value):
        msg = IpcMessage("cmd", value)
        success, reply = self._send_message(msg)
        if success:
            return reply
        else:
            self._raise_reply_error(msg, reply)

    def send_configuration(self, target, content, valid_error=None):
        msg = IpcMessage("cmd", "configure")
        if not target:
            for key in content:
                msg.set_param(key, content[key])
        else:
            msg.set_param(target, content)
        success, reply = self._send_message(msg)
        if not success and None not in [reply, valid_error]:
            if reply["params"]["error"] != valid_error:
                self._raise_reply_error(msg, reply)
            else:
                self.logger.debug("Got valid error for request %s: %s",
                                  msg, reply)
        return success, reply

    def send_configuration_dict(self, **kwargs):
        msg = IpcMessage("cmd", "configure")
        for key, value in kwargs.items():
            msg.set_param(key, value)
        return self._send_message(msg)

    def _read_message(self, timeout):
        pollevts = self.ctrl_channel.poll(timeout)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            return reply
