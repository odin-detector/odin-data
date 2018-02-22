import logging
from threading import RLock

import zmq

from odin_data.ipc_channel import IpcChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException


class IpcClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"
    
    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip_address, port):
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(
            IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        
        self.message_id = 0

        self._lock = RLock()

    def _send_message(self, msg, timeout):
        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        self.logger.debug("Sending control message:\n%s", msg.encode())
        with self._lock:
            self.ctrl_channel.send(msg.encode())
            expected_id = msg.get_msg_id()
            id = None
            while not id == expected_id:
                pollevts = self.ctrl_channel.poll(timeout)
    
                if pollevts == zmq.POLLIN:
                    reply = IpcMessage(from_str=self.ctrl_channel.recv())
                    id = reply.get_msg_id()
                    if not id == expected_id:
                        self.logger.warn("Dropping reply message with id [" + str(id) + "] as was expecting [" + str(expected_id) + "]")
                        continue
                    if reply.is_valid() and reply.get_msg_type() == IpcMessage.ACK:
                        self.logger.debug("Request successful: %s", reply)
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

    def send_request(self, value, timeout=1000):
        msg = IpcMessage("cmd", value)
        success, reply = self._send_message(msg, timeout)
        if success:
            return reply
        else:
            self._raise_reply_error(msg, reply)

    def send_configuration(self, content, target=None, valid_error=None, timeout=1000):
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        success, reply = self._send_message(msg, timeout)
        if not success and None not in [reply, valid_error]:
            if reply["params"]["error"] != valid_error:
                self._raise_reply_error(msg, reply)
            else:
                self.logger.debug("Got valid error for request %s: %s",
                                  msg, reply)
        return success, reply

    def _read_message(self, timeout):
        pollevts = self.ctrl_channel.poll(timeout)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            return reply
