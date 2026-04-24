import json
from unittest import TestCase
from unittest.mock import DEFAULT, patch

import zmq
from odin_data.control.ipc_client import IpcClient, IpcMessageException


@patch("odin_data.control.ipc_client.IpcChannel")
class TestIpcClient(TestCase):
    def test_send_request(self, ipcChannel):

        client = IpcClient("127.0.0.1", 10000)

        # Assert IpcChannel constructed
        ipcChannel.assert_called_once()

        # Set ipcChannel to the created Mock
        ipcChannel = client.ctrl_channel

        self.send_msg = ""

        def side_effect(*args, **kwargs):
            self.send_msg = args[0]
            return DEFAULT

        ipcChannel.send.side_effect = side_effect
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "test_request", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        # Send a request through the client
        try:
            assert client.send_request("test_request")
        except IpcMessageException:
            self.fail("send_request() raised IpcMessageException unexpectedly")

        ipcChannel.send.assert_called()
        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        # Assert that the request we sent ended up as the msg_value of the message
        # sent to the IpcChannel mock
        msg = json.loads(self.send_msg)
        assert msg["msg_val"] == "test_request"

        # Send the same request, this time it should be rejected as the msg ID will
        # not match
        ipcChannel.recv.return_value = b'{"id": 1, "msg_type": "nack", "msg_val": "test_request", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'
        with self.assertRaises(IpcMessageException):
            client.send_request("test_request")

    def test_send_configuration(self, ipcChannel):

        client = IpcClient("127.0.0.1", 10000)

        # Assert IpcChannel constructed
        ipcChannel.assert_called_once()

        # Set ipcChannel to the created Mock
        ipcChannel = client.ctrl_channel

        self.send_msg = ""

        def side_effect(*args, **kwargs):
            self.send_msg = args[0]
            return DEFAULT

        ipcChannel.send.side_effect = side_effect
        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        # Send a request through the client
        try:
            assert client.send_configuration({"TestParam1": 1, "TestParam2": 2.0})
        except IpcMessageException:
            self.fail("send_request() raised IpcMessageException unexpectedly")

        ipcChannel.send.assert_called()
        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        # Assert that the request we sent ended up as the msg_value of the message
        # sent to the IpcChannel mock
        msg = json.loads(self.send_msg)
        assert msg["msg_val"] == "configure"
        assert msg["params"]["TestParam1"] == 1
        assert msg["params"]["TestParam2"] == 2.0

        # Test invalid error response
        ipcChannel.recv.return_value = b'{"id": 1, "msg_type": "nack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {"error": "test error"}}'
        with self.assertRaises(IpcMessageException):
            assert client.send_configuration(
                target="TestParam1", content=1, valid_error="false error"
            )

        # Test valid error response
        ipcChannel.recv.return_value = b'{"id": 2, "msg_type": "nack", "msg_val": "", "timestamp": "2026-03-12T17:42:30.857274", "params": {"error": "test error"}}'
        try:
            assert client.send_configuration(
                {"TestParam1": 1, "TestParam2": 2.0}, valid_error="test error"
            )
        except IpcMessageException:
            self.fail("send_request() raised IpcMessageException unexpectedly")

    def test_read_message(self, ipcChannel):

        client = IpcClient("127.0.0.1", 10000)

        # Assert IpcChannel constructed
        ipcChannel.assert_called_once()

        # Set ipcChannel to the created Mock
        ipcChannel = client.ctrl_channel

        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.return_value = b'{"id": 0, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}'

        # Call read message
        try:
            msg = client._read_message(1.0)
            assert msg.get_msg_val() == "test"
            assert msg.get_msg_id() == 0
        except IpcMessageException:
            self.fail("send_request() raised IpcMessageException unexpectedly")

        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

    def test_check_for_valid_message(self, ipcChannel):
        client = IpcClient("127.0.0.1", 10000)

        # Assert IpcChannel constructed
        ipcChannel.assert_called_once()

        # Set ipcChannel to the created Mock
        ipcChannel = client.ctrl_channel

        ipcChannel.poll.return_value = zmq.POLLIN
        ipcChannel.recv.side_effect = [
            b'{"id": 99, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
            b'{"id": 0, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
            b'{"id": 1, "msg_type": "ack", "msg_val": "test", "timestamp": "2026-03-12T17:42:30.857274", "params": {}}',
        ]

        # Check that if a request is sent and the reply has an incorrect ID, then
        # messages continue to be checked until the correct ID is found.
        try:
            assert client.send_request("test_request")
        except IpcMessageException:
            self.fail("send_request() raised IpcMessageException unexpectedly")

        ipcChannel.poll.assert_called()
        ipcChannel.recv.assert_called()

        # Assert that an incorrect poll return results in a raised Exception
        ipcChannel.poll.return_value = None
        with self.assertRaises(IpcMessageException):
            assert client.send_request("test_request")
