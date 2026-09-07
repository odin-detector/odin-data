from unittest.mock import patch

import pytest
from odin_data.control.ipc_message import IpcMessage, IpcMessageException
from odin_data.control.ipc_tornado_client import IpcTornadoClient


@pytest.fixture
def tornado_channel():
    with patch("odin_data.control.ipc_tornado_client.IpcTornadoChannel") as mock:
        yield mock


@pytest.fixture
def client(tornado_channel):
    return IpcTornadoClient("1.1.1.1", 9999)


class TestIpcTornadoClient:
    def test_ipc_tornado_client(self, client, tornado_channel):
        assert client._ip_address == "1.1.1.1"
        assert client._port == 9999
        tornado_channel.assert_called_once_with(tornado_channel.CHANNEL_TYPE_DEALER)

        assert client.parameters == {
            client.IPC_VAL_STATUS: {
                client.STATUS_PARAMS_KEY: {client.CLIENT_CONNECTED: False}
            }
        }
        assert client.connected() is False

        reply_str = '{"id": 0, "msg_type": "cmd", "msg_val": "request_version", "params": {"version": "1.0.0"}}'
        client.send_request("request_version")
        client._callback([reply_str])
        assert client.parameters["version"] == "1.0.0"

        # No plugins loaded config reply
        reply_str = '{"params": {"ctrl_endpoint": "tcp://0.0.0.0:10004", "meta_endpoint": "tcp://*:10008", "config_ts": -1}, "msg_type": "ack", "msg_val": "request_configuration", "id": 1, "timestamp": "2026-09-07T17:22:40.026023"}'
        client.send_request("request_configuration")
        client._callback([reply_str])
        assert client.parameters["config"] == {
            "config_ts": -1,
        }

        # Plugin(s) loaded config reply
        reply_str = '{"id": 2, "msg_type": "cmd", "msg_val": "request_configuration", "params": { "plugins": {"names": ["test_config_item"] }, "config_ts": 12345, "test_config_item": "Test1"}}'
        client.send_request("request_configuration")
        client._callback([reply_str])
        assert client.parameters["config"] == {
            "config_request": {"test_config_item": "Test1"},
            "config_ts": 12345,
        }

        # No plugins loaded status reply
        reply_str = '{"params": {"status_ts": -1}, "msg_type": "cmd", "msg_val": "status", "id": 3, "timestamp": "2026-09-07T17:48:43.421252"}'
        client.send_request("status")
        client._callback([reply_str])
        assert client.parameters["status"] == {
            "status_request": {
                "timestamp": "2026-09-07T17:48:43.421252",
                "error": [],
                "connected": True,
            },
            "status_ts": -1,
        }
        assert client.connected() is True

        # Plugin(s) loaded status reply
        reply_str = '{"id": 4, "msg_type": "cmd", "msg_val": "status", "timestamp": "00:00:00.00", "params": { "plugins": {"names": ["test_config_item"] }, "status_ts": 12345, "test_config_item": "Test1"}}'
        client.send_request("status")
        client._callback([reply_str])
        assert client.parameters["status"] == {
            "status_request": {
                "plugins": {"names": ["test_config_item"]},
                "test_config_item": "Test1",
                "timestamp": "00:00:00.00",
                "error": [],
                "connected": True,
            },
            "status_ts": 12345,
        }
        assert client.connected() is True

        reply_str = '{"id": 5, "msg_type": "cmd", "msg_val": "request_commands", "params": {"test_command_item": "Test3"}}'
        client.send_request("request_commands")
        client._callback([reply_str])
        assert client.parameters["commands"] == {"test_command_item": "Test3"}

        reply_str = '{"id": 6, "msg_type": "nack", "msg_val": "configure", "params": {"test_config_fail": "Test4", "error": "test error"}}'
        client.send_configuration({"item1": "value1"})
        client._callback([reply_str])
        rejected = client.read_rejected_configs()
        assert len(rejected) == 1
        assert rejected[6].get_msg_id() == IpcMessage(from_str=reply_str).get_msg_id()
        assert client.check_for_rejection(6) is True

        reply_str = '{"id": 7, "msg_type": "cmd", "msg_val": "status", "timestamp": "00:00:00.00", "params": { "plugins": {"names": ["test_status_item"] }, "test_status_item": "Test2"}}'
        client.send_request("status")
        client._callback([reply_str])
        assert client.parameters["status"]["status_request"]["error"] == ["test error"]

        client.clear_rejected_configs()
        assert len(client.read_rejected_configs()) == 0

        reply_str = '{"id": 8, "msg_type": "nack", "msg_val": "execute", "params": {"test_command_fail": "Test5"}}'
        client.execute_command("text", "execute")
        client._callback([reply_str])
        rejected = client.read_rejected_commands()
        assert len(rejected) == 1
        assert rejected[8].get_msg_id() == IpcMessage(from_str=reply_str).get_msg_id()

        client.clear_rejected_commands()
        assert len(client.read_rejected_commands()) == 0

        client._monitor_callback({"event": tornado_channel.CONNECTED})
        assert client.connected() is True
        client._monitor_callback({"event": tornado_channel.DISCONNECTED})
        assert client.connected() is False

        with pytest.raises(IpcMessageException):
            client._raise_reply_error("TestMsg", "TestReply")
        with pytest.raises(IpcMessageException):
            client._raise_reply_error("TestMsg", None)

        timeout = client.wait_for_response(9)
        assert timeout is False
        client.send_request("status")
        timeout = client.wait_for_response(9)
        assert timeout is True
