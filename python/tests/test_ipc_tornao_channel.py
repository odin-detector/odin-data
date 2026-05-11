from unittest import TestCase
from unittest.mock import MagicMock, patch

from odin_data.control.ipc_tornado_channel import IpcTornadoChannel
from zmq.utils.monitor import parse_monitor_message


@patch("odin_data.control.ipc_channel.IpcChannel.__init__")
@patch("odin_data.control.ipc_channel.IpcChannel.send")
@patch("odin_data.control.ipc_tornado_channel.ZMQStream")
class TestIpcTornadoChannel(TestCase):
    def test_ipc_tornado_channel(self, zmq_stream, ipc_send, ipc_init):
        client = IpcTornadoChannel(0)

        ipc_init.assert_called_once()

        # Send some data
        client.send("data1")
        # Assert the parent class is called with the data
        ipc_send.assert_called_once_with(b"data1")

        # Mock the channel stream object
        client._stream = MagicMock()
        client._stream.send = MagicMock()
        # Send some data again
        client.send("data2")
        # Assert the stream is called with the data
        client._stream.send.assert_called_once_with(b"data2")

        # Register a monitor
        client.socket = MagicMock()
        client.socket.get_monitor_socket = MagicMock(return_value="test_socket")
        client.register_monitor("test_cb")
        self.assertEquals(client._monitor_socket, "test_socket")
        self.assertEquals(client._monitor_callback, "test_cb")
        zmq_stream.assert_called_once_with("test_socket")

        # Mock the callback and verify it is called when the callback is triggered
        client._monitor_callback = MagicMock()
        client._internal_monitor_callback(
            [bytes([0, 1, 0, 0, 0, 2]), bytes([0, 0, 0, 0])]
        )
        client._monitor_callback.assert_called_once_with(
            parse_monitor_message([bytes([0, 1, 0, 0, 0, 2]), bytes([0, 0, 0, 0])])
        )
