import sys
import zmq
from zmq.utils.strtypes import unicode, cast_bytes
import uuid


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
    CHANNEL_TYPE_DEALER = zmq.DEALER
    CHANNEL_TYPE_ROUTER = zmq.ROUTER

    def __init__(self, channel_type, endpoint=None, context=None, identity=None):
        self.channel_type = channel_type
        if endpoint:
            self.endpoint = endpoint
        self.context = context or zmq.Context().instance()
        self.socket = self.context.socket(self.channel_type)
        if self.channel_type == self.CHANNEL_TYPE_DEALER:
            if identity is None:
                identity = str(uuid.uuid4())
            self.socket.setsockopt(zmq.IDENTITY, cast_bytes(identity))

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
        if isinstance(data, unicode):
            data = cast_bytes(data)
        self.socket.send(data)

    def recv(self):
        data = list(map(self._cast_str, self.socket.recv_multipart()))

        if self.channel_type == self.CHANNEL_TYPE_ROUTER:
            identity = data.pop(0)
            return (identity, data)
        else:
            return data[0]

    def poll(self, timeout=None):
        pollevts = self.socket.poll(timeout)
        return pollevts

    def subscribe(self, topic=b''):
        self.socket.setsockopt(zmq.SUBSCRIBE, topic)

    def _cast_str(self, s, encoding='utf8', errors='strict'):
        """Cast bytes or unicode to unicode for Python 3 strings."""
        if sys.version_info[0] < 3:
            return s
        elif isinstance(s, bytes):
            return s.decode(encoding, errors)
        elif isinstance(s, unicode):
            return s
        else:
            raise IpcChannelException("Expected unicode or bytes, got %r" % s)
