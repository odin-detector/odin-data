"""Implementation of odin_data inter-process communication channels for use
within Tornado IO Loop.

This module implements the ODIN data IpcChannel class for inter-process
communication via ZeroMQ sockets.

Tim Nicholls, STFC Application Engineering Group
"""
from zmq.eventloop.zmqstream import ZMQStream
from zmq.utils.monitor import parse_monitor_message
from zmq.utils.strtypes import unicode, cast_bytes

from odin_data.ipc_channel import IpcChannel


class IpcTornadoChannel(IpcChannel):
    """Inter-process communication channel class for use with Tornado IO Loop.

    """
    CONNECTED = IpcChannel.EVENT_ACCEPTED
    DISCONNECTED = IpcChannel.EVENT_DISCONNECTED

    def __init__(self, channel_type, endpoint=None, context=None, identity=None):
        """Initalise the IpcChannel object.

        :param channel_type: ZeroMQ socket type, using CHANNEL_TYPE_xxx constants
        :param endpoint: URI of channel endpoint, can be specified later
        :param context: ZeroMQ context, will be initialised if not given
        :param identity: channel identity for DEALER type sockets
        """
        super(IpcTornadoChannel, self).__init__(channel_type, endpoint, context, identity)
        self._callback = None
        self._monitor_callback = None
        self._stream = None

    def register_callback(self, callback):
        """Register a callback with this IpcChannel.  This will result in the
        construction of a ZMQStream and the callback will be registered with
        the stream object.

        :param: data to send on channel
        """
        self._callback = callback
        if not self._stream:
            self._stream = ZMQStream(self.socket)
        self._stream.on_recv(callback)

    def send(self, data):
        """Send data to the IpcChannel.

        :param: data to send on channel
        """

        # If the data are unicode (like all Python3 native strings), convert to a
        # byte stream to be sent on the socket
        if isinstance(data, unicode):
            data = cast_bytes(data)

        # If a Stream is registered send the data out on the tornado IO Loop
        if self._stream:
            self._stream.send(data)
        else:
            super(IpcTornadoChannel, self).send(data)

    def send_multipart(self, data):
        """
        Send data to the IpcChannel, in multiple parts.
        :param: data to send, as an iterable object
        """

        for idx, part in enumerate(data):
            if isinstance(part, unicode):
                data[idx] = cast_bytes(part)

        if self._stream:
            self._stream.send_multipart(data)
        else:
            super(IpcTornadoChannel, self).send_multipart(data)

    def register_monitor(self, callback):
        self._monitor_callback = callback
        self._monitor_socket = self.socket.get_monitor_socket(IpcChannel.EVENT_ACCEPTED | IpcChannel.EVENT_DISCONNECTED)
        # Create the socket
        self._monitor_stream = ZMQStream(self._monitor_socket)
        self._monitor_stream.on_recv(self._internal_monitor_callback)

    def _internal_monitor_callback(self, msg):
        if self._monitor_callback is not None:
            self._monitor_callback(parse_monitor_message(msg))

