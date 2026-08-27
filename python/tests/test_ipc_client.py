import json
import pytest
from unittest.mock import DEFAULT, patch

import zmq
from odin_data.control.ipc_client import IpcClient, IpcMessageException


@pytest.fixture
def ipc_channel():
    with patch("odin_data.control.ipc_client.IpcChannel") as mock:
        yield mock


@pytest.fixture
def client(ipc_channel):
    return IpcClient("127.0.0.1", 10000)


def make_side_effect(container):
    def side_effect(*args, **kwargs):
        container["send_msg"] = args[0]
        return DEFAULT
    return side_effect


class TestIpcClient:
    def test_send_request(self, client, ipc_channel):
        ipc_channel.assert_called_once()

        ipcChannel = client.ctrl_channel
        sent = {}
        ipcChannel.send.side_effect = make_side_effect(sent)
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "test_request", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        assert client.send_request("test_request")

        ipcChannel.send.assert_called()
        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        msg = json.loads(sent["send_msg"])
        assert msg["msg_val"] == "test_request"

        ipcChannel.recv.return_value = b'{"id": 1, "msg_type": "nack", "msg_val": "test_request", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'
        with pytest.raises(IpcMessageException):
            client.send_request("test_request")

    def test_send_configuration(self, client, ipc_channel):
        ipc_channel.assert_called_once()

        ipcChannel = client.ctrl_channel
        sent = {}
        ipcChannel.send.side_effect = make_side_effect(sent)
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        assert client.send_configuration({"TestParam1": 1, "TestParam2": 2.0})

        ipcChannel.send.assert_called()
        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        msg = json.loads(sent["send_msg"])
        assert msg["msg_val"] == "configure"
        assert msg["params"]["TestParam1"] == 1
        assert msg["params"]["TestParam2"] == 2.0

        ipcChannel.recv.return_value = b'{"id": 1, "msg_type": "nack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {"error": "test error"}}'
        with pytest.raises(IpcMessageException):
            client.send_configuration(target="TestParam1", content=1, valid_error="false error")

        ipcChannel.recv.return_value = b'{"id": 2, "msg_type": "nack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {"error": "test error"}}'
        assert client.send_configuration({"TestParam1": 1, "TestParam2": 2.0}, valid_error="test error")

    def test_read_message(self, client, ipc_channel):
        ipc_channel.assert_called_once()

        ipcChannel = client.ctrl_channel
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        msg = client._read_message(1.0)
        assert msg.get_msg_val() == "test"
        assert msg.get_msg_id() == 0

        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

    def test_check_for_valid_message(self, client, ipc_channel):
        ipc_channel.assert_called_once()

        ipcChannel = client.ctrl_channel
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.side_effect = [
            b'{"id": 99, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
            b'{"id": 0, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
            b'{"id": 1, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
        ]

        assert client.send_request("test_request")

        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        ipcChannel.poll.return_value = None
        with pytest.raises(IpcMessageException):
            client.send_request("test_request")
