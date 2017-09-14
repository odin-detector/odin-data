"""Implementation of odin_data inter-process communication channels.

This module implements the ODIN data IpcChannel class for inter-process
communication via ZeroMQ sockets.

Tim Nicholls, STFC Application Engineering Group
"""
import sys
import random
import zmq
from zmq.utils.strtypes import unicode, cast_bytes


class IpcChannelException(Exception):
    """Exception class for IpcChannel.

    This class implements a simple exception for use with IpcChannel, providing
    a readable message and optionsal error code.
    """

    def __init__(self, msg, errno=None):
        """Initalise the exception.

        :param msg: readable message associated with the exception
        :param errno: optional error number assocated with the exception
        """
        super(IpcChannelException, self).__init__()
        self.msg = msg
        self.errno = errno

    def __str__(self):
        """Return string representation of the exception message."""
        return str(self.msg)


class IpcChannel(object):
    """Inter-process communication channel class.

    This class provides ZeroMQ-based interprocess communication channels to
    odin-data.
    """
    # pylint: disable=no-member
    CHANNEL_TYPE_PAIR = zmq.PAIR
    CHANNEL_TYPE_REQ = zmq.REQ
    CHANNEL_TYPE_SUB = zmq.SUB
    CHANNEL_TYPE_PUB = zmq.PUB
    CHANNEL_TYPE_DEALER = zmq.DEALER
    CHANNEL_TYPE_ROUTER = zmq.ROUTER

    POLLIN = zmq.POLLIN
    POLLOUT = zmq.POLLOUT
    POLLERR = zmq.POLLERR
    # pylint: enable=no-member

    def __init__(self, channel_type, endpoint=None, context=None, identity=None):
        """Initalise the IpcChannel object.

        :param channel_type: ZeroMQ socket type, using CHANNEL_TYPE_xxx constants
        :param endpoint: URI of channel endpoint, can be specified later
        :param context: ZeroMQ context, will be initialised if not given
        :param identity: channel identity for DEALER type sockets
        """
        # Initalise channel type and endpoint if given
        self.channel_type = channel_type
        if endpoint:
            self.endpoint = endpoint

        # Initialise the ZeroMQ context or obtain the current instance
        self.context = context or zmq.Context().instance()

        # Create the socket
        self.socket = self.context.socket(self.channel_type)

        # If the socket type is DEALER, set the identity, chosing a random
        # UUID4 value if not specified
        if self.channel_type == self.CHANNEL_TYPE_DEALER:
            if identity is None:
                identity = "{:04x}-{:04x}".format(
                    random.randrange(0x10000), random.randrange(0x10000)
                )
            self.socket.setsockopt(zmq.IDENTITY, cast_bytes(identity))  # pylint: disable=no-member

    def bind(self, endpoint=None):
        """Bind the IpcChannel to an endpoint.

        :param: endpoint: endpoint URI to use, otherwise use initialised value
        """
        if endpoint:
            self.endpoint = endpoint

        self.socket.bind(self.endpoint)

    def connect(self, endpoint=None):
        """Connect the IpcChannel to an endpoint.

        :param: endpoint: endpoint URI to use, otherwise use initialised value
        """
        if endpoint:
            self.endpoint = endpoint

        self.socket.connect(self.endpoint)

    def close(self):
        """Close the IpcChannel socket."""
        self.socket.close()

    def send(self, data):
        """Send data to the IpcChannel.

        :param: data to send on channel
        """
        # If the data is unicode (like all Python3 native strings), convert to a
        # byte stream to be sent on the socket
        if isinstance(data, unicode):
            data = cast_bytes(data)

        # Send the data
        self.socket.send(data)

    def recv(self):
        """Recieve data from the IpcChannel.

        :return: returns either the data received, or, in the case of DEALER/ROUTER
        channels, a tuple of DEALER channel identity and data received
        """
        # Use multipart receive to cope with data coming from DEALER sockets,
        # where the message is prefixed by the socket identity. Convert incoming
        # data back to native strings if required
        data = list(map(_cast_str, self.socket.recv_multipart()))

        # If our local channel is a router, the remote endpoint should (must) be a
        # dealer, in which case pop the identity off the front of the data and
        # return both.
        if self.channel_type == self.CHANNEL_TYPE_ROUTER:
            identity = data.pop(0)
            return (identity, data)

        return data[0]

    def poll(self, timeout=None):
        """Poll the IpcChannel socket for I/O events.

        :param timeout: poll timeout in milliseconds
        :return list of poll events for the socket
        """
        pollevts = self.socket.poll(timeout)
        return pollevts

    def subscribe(self, topic=b''):
        """Set the topic subscription for SUB sockets.

        :param topic: topic to subscribe to
        """
        if self.channel_type == self.CHANNEL_TYPE_SUB:
            self.socket.setsockopt(zmq.SUBSCRIBE, topic)  # pylint: disable=no-member
        else:
            raise IpcChannelException("Attmped to set topic subscription on non-SUB channel socket")


def _cast_str(the_str, encoding='utf8', errors='strict'):
    """Cast bytes or unicode to unicode for Python 3 strings.

    :param the_str: string to convert
    :param encoding: unicode string encoding to use, default is utf8
    :param errors: decoder error handling scheme
    :return: returns string cast to appropriate type
    """
    if sys.version_info[0] < 3:
        return the_str
    elif isinstance(the_str, bytes):
        return the_str.decode(encoding, errors)
    elif isinstance(the_str, unicode):
        return the_str
    else:
        raise IpcChannelException("Expected unicode or bytes, got %r" % the_str)
