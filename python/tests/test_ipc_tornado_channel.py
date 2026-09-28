import pytest
from unittest.mock import MagicMock, patch

from odin_data.control.ipc_tornado_channel import IpcTornadoChannel
from zmq.utils.monitor import parse_monitor_message


@pytest.fixture
def tornado_channel_mocks():
    with patch("odin_data.control.ipc_channel.IpcChannel.__init__") as ipc_init, \
         patch("odin_data.control.ipc_channel.IpcChannel.send") as ipc_send, \
         patch("odin_data.control.ipc_tornado_channel.ZMQStream") as zmq_stream:
        yield ipc_init, ipc_send, zmq_stream


class TestIpcTornadoChannel:
    def test_ipc_tornado_channel(self, tornado_channel_mocks):
        ipc_init, ipc_send, zmq_stream = tornado_channel_mocks

        client = IpcTornadoChannel(0)
        ipc_init.assert_called_once()

        client.send("data1")
        ipc_send.assert_called_once_with(b"data1")

        client._stream = MagicMock()
        client._stream.send = MagicMock()
        client.send("data2")
        client._stream.send.assert_called_once_with(b"data2")

        client.socket = MagicMock()
        client.socket.get_monitor_socket = MagicMock(return_value="test_socket")
        client.register_monitor("test_cb")
        assert client._monitor_socket == "test_socket"
        assert client._monitor_callback == "test_cb"
        zmq_stream.assert_called_once_with("test_socket")

        client._monitor_callback = MagicMock()
        client._internal_monitor_callback(
            [bytes([0, 1, 0, 0, 0, 2]), bytes([0, 0, 0, 0])]
        )
        client._monitor_callback.assert_called_once_with(
            parse_monitor_message([bytes([0, 1, 0, 0, 0, 2]), bytes([0, 0, 0, 0])])
        )
