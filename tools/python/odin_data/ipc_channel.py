import zmq


class IpcChannelException(Exception):

    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno

    def __str__(self):
        return str(self.msg)


class IpcChannel(object):

    CHANNEL_TYPE_PAIR = zmq.PAIR
    CHANNEL_TYPE_REQ = zmq.REQ
    CHANNEL_TYPE_SUB = zmq.SUB
    CHANNEL_TYPE_PUB = zmq.PUB

    def __init__(self, channel_type, endpoint=None, context=None):
        self.context = context or zmq.Context().instance()
        self.socket = self.context.socket(channel_type)

        if endpoint:
            self.endpoint = endpoint

    def bind(self, endpoint=None):
        if endpoint:
            self.endpoint = endpoint
        self.socket.bind(self.endpoint)

    def connect(self, endpoint=None):
        if endpoint:
            self.endpoint = endpoint
        self.socket.connect(self.endpoint)

    def close(self):
        self.socket.close()

    def send(self, data):
        self.socket.send_string(data)

    def recv(self):
        data = self.socket.recv()
        return data

    def poll(self, timeout=None):
        pollevts = self.socket.poll(timeout)
        return pollevts

    def subscribe(self, topic=b''):
        self.socket.setsockopt(zmq.SUBSCRIBE, topic)
