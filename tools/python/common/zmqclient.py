import zmq

from common import IpcChannel, IpcMessage


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

    def _send_message(self, msg):
        with self._lock:
            self.ctrl_channel.send(msg.encode())
            pollevts = self.ctrl_channel.poll(1000)
            if pollevts == zmq.POLLIN:
                reply = IpcMessage(from_str=self.ctrl_channel.recv())
                if reply.is_valid() and reply.get_msg_type() == IpcMessage.ACK:
                    return reply
        return False

    def send_request(self, value):
        msg = IpcMessage("cmd", value)
        return self._send_message(msg)

    def send_configuration(self, target, content):
        msg = IpcMessage("cmd", "configure")
        msg.set_param(target, content)
        return bool(self._send_message(msg))

    def send_configuration_dict(self, **kwargs):
        msg = IpcMessage("cmd", "configure")
        for key, value in kwargs.items():
            msg.set_param(key, value)
        self._send_message(msg)

    def _read_message(self, timeout):
        pollevts = self.ctrl_channel.poll(timeout)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            return reply