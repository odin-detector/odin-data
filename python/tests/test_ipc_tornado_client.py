from unittest import TestCase
from unittest.mock import patch

from odin_data.control.ipc_message import IpcMessage, IpcMessageException
from odin_data.control.ipc_tornado_client import IpcTornadoClient


@patch("odin_data.control.ipc_tornado_client.IpcTornadoChannel")
class TestIpcTornadoClient(TestCase):
    def test_ipc_tornado_client(self, tornado_channel):
        client = IpcTornadoClient("1.1.1.1", 9999)

        self.assertEquals(client._ip_address, "1.1.1.1")
        self.assertEquals(client._port, 9999)
        tornado_channel.assert_called_once_with(tornado_channel.CHANNEL_TYPE_DEALER)

        # Verify default parameter dict
        self.assertEquals(
            client.parameters, {client.IPC_VAL_STATUS: {client.CLIENT_CONNECTED: False}}
        )

        # Verify connected method returns the connected parameter
        self.assertEquals(client.connected(), False)

        reply_str = '{"id": 0, "msg_type": "cmd", "msg_val": "request_version", "params": {"version": "1.0.0"}}'
        client.send_request("request_version")
        client._callback([reply_str])
        self.assertEquals(client.parameters["version"], "1.0.0")

        reply_str = '{"id": 1, "msg_type": "cmd", "msg_val": "request_configuration", "params": {"test_config_item": "Test1"}}'
        client.send_request("request_configuration")
        client._callback([reply_str])
        self.assertEquals(client.parameters["config"], {"test_config_item": "Test1"})

        reply_str = '{"id": 2, "msg_type": "cmd", "msg_val": "status", "timestamp": "00:00:00.00", "params": {"test_status_item": "Test2"}}'
        client.send_request("status")
        client._callback([reply_str])
        self.assertEquals(
            client.parameters["status"],
            {
                "test_status_item": "Test2",
                "timestamp": "00:00:00.00",
                "error": [],
                "connected": True,
            },
        )
        self.assertEquals(client.connected(), True)

        reply_str = '{"id": 3, "msg_type": "cmd", "msg_val": "request_commands", "params": {"test_command_item": "Test3"}}'
        client.send_request("request_commands")
        client._callback([reply_str])
        self.assertEquals(
            client.parameters["commands"],
            {
                "test_command_item": "Test3",
            },
        )

        # Send a config that gets rejected
        reply_str = '{"id": 4, "msg_type": "nack", "msg_val": "configure", "params": {"test_config_fail": "Test4", "error": "test error"}}'
        client.send_configuration({"item1": "value1"})
        client._callback([reply_str])
        rejected = client.read_rejected_configs()
        self.assertEquals(len(rejected), 1)
        self.assertEquals(
            rejected[4].get_msg_id(), IpcMessage(from_str=reply_str).get_msg_id()
        )
        # Check the rejected config can be searched by ID
        self.assertEquals(client.check_for_rejection(4), True)

        # Send a status update request to force an error check
        reply_str = '{"id": 5, "msg_type": "cmd", "msg_val": "status", "timestamp": "00:00:00.00", "params": {"test_status_item": "Test2"}}'
        client.send_request("status")
        client._callback([reply_str])
        # Verify the error has been registered
        self.assertEquals(client.parameters["status"]["error"], ["test error"])

        # Clear the rejected configs and verify the dict is clear
        client.clear_rejected_configs()
        rejected = client.read_rejected_configs()
        self.assertEquals(len(rejected), 0)

        # Send a command that gets rejected
        reply_str = '{"id": 6, "msg_type": "nack", "msg_val": "execute", "params": {"test_command_fail": "Test5"}}'
        client.execute_command("text", "execute")
        client._callback([reply_str])
        rejected = client.read_rejected_commands()
        self.assertEquals(len(rejected), 1)
        self.assertEquals(
            rejected[6].get_msg_id(), IpcMessage(from_str=reply_str).get_msg_id()
        )

        # Clear the rejected configs and verify the dict is clear
        client.clear_rejected_commands()
        rejected = client.read_rejected_commands()
        self.assertEquals(len(rejected), 0)

        # Verify monitoring of sockets sets the connected status
        client._monitor_callback({"event": tornado_channel.CONNECTED})
        self.assertEquals(client.connected(), True)
        client._monitor_callback({"event": tornado_channel.DISCONNECTED})
        self.assertEquals(client.connected(), False)

        # Verify correct exceptions are raised
        with self.assertRaises(IpcMessageException):
            client._raise_reply_error("TestMsg", "TestReply")
        with self.assertRaises(IpcMessageException):
            client._raise_reply_error("TestMsg", None)

        timeout = client.wait_for_response(7)
        self.assertEquals(timeout, False)
        client.send_request("status")
        timeout = client.wait_for_response(7)
        self.assertEquals(timeout, True)
