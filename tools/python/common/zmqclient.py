import zmq

from common import IpcChannel, IpcMessage, IpcMessageException


class ZMQClient(object):

    IP = None
    CTRL_PORT = None
    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    def __init__(self, ip_address, lock, server_rank=0):
        port = self.CTRL_PORT + server_rank * 1000

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address,
                                                           PORT=port)
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ctrl_channel.connect(self.ctrl_endpoint)

        self._lock = lock

    def _send_message(self, msg, timeout=1000):
        with self._lock:
            self.ctrl_channel.send(msg.encode())
            pollevts = self.ctrl_channel.poll(timeout)

        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            if reply.is_valid() \
               and reply.get_msg_type() == IpcMessage.ACK:
                return True, reply.attrs
            else:
                return False, reply.attrs
        else:
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
        msg.set_param(target, content)
        success, reply = self._send_message(msg)
        if not success:
            if reply["params"]["error"] != valid_error:
                self._raise_reply_error(msg, reply)
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
