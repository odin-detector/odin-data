import os
import tempfile
from unittest import TestCase
from unittest.mock import MagicMock, patch

from odin_data.client import OdinDataClient


@patch("odin_data.client.IpcChannel")
class TestClientProgram(TestCase):
    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_client(self, arg_parse, ipc_channel):
        client = OdinDataClient()

        arg_parse.assert_called_once()
        ipc_channel.assert_called_once_with(ipc_channel.CHANNEL_TYPE_DEALER)

        self.assertEqual(client._msg_id, 0)
        new_id = client._next_msg_id()
        self.assertEqual(new_id, 1)
        self.assertEqual(client._msg_id, 1)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_shutdown(self, arg_parse, ipc_channel):
        client = OdinDataClient()
        client.args.config_file = None
        client.args.request_config = False
        client.args.request_commands = False
        client.args.status = False
        client.args.shutdown = True

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        self.assertEqual('"id": 1' in args[0], True)
        self.assertEqual('"msg_type": "cmd"' in args[0], True)
        self.assertEqual('"msg_val": "shutdown"' in args[0], True)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_status(self, arg_parse, ipc_channel):
        client = OdinDataClient()
        client.args.config_file = None
        client.args.request_config = False
        client.args.request_commands = False
        client.args.status = True
        client.args.shutdown = False

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        self.assertEqual('"id": 1' in args[0], True)
        self.assertEqual('"msg_type": "cmd"' in args[0], True)
        self.assertEqual('"msg_val": "status"' in args[0], True)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_request_command(self, arg_parse, ipc_channel):
        client = OdinDataClient()
        client.args.config_file = None
        client.args.request_config = False
        client.args.request_commands = True
        client.args.status = False
        client.args.shutdown = False

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        self.assertEqual('"id": 1' in args[0], True)
        self.assertEqual('"msg_type": "cmd"' in args[0], True)
        self.assertEqual('"msg_val": "request_commands"' in args[0], True)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_request_config(self, arg_parse, ipc_channel):
        client = OdinDataClient()
        client.args.config_file = None
        client.args.request_config = True
        client.args.request_commands = False
        client.args.status = False
        client.args.shutdown = False

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        self.assertEqual('"id": 1' in args[0], True)
        self.assertEqual('"msg_type": "cmd"' in args[0], True)
        self.assertEqual('"msg_val": "request_configuration"' in args[0], True)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_config_cmd(self, arg_parse, ipc_channel):
        client = OdinDataClient()
        tmp_file = tempfile.NamedTemporaryFile(
            mode="w",
            buffering=-1,
            encoding=None,
            newline=None,
            suffix=None,
            prefix=None,
            dir=None,
            errors=None,
            delete=False,
        )
        tmp_file.write('{"test": "val"}\n')
        file_name = tmp_file.name
        tmp_file.close()
        r_file = open(file_name, "r")
        client.args.config_file = r_file
        client.args.request_config = False
        client.args.request_commands = False
        client.args.status = False
        client.args.shutdown = False

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        self.assertEqual('"id": 1' in args[0], True)
        self.assertEqual('"msg_type": "cmd"' in args[0], True)
        self.assertEqual('"msg_val": "configure"' in args[0], True)
        self.assertEqual('"params": {"test": "val"}' in args[0], True)

        r_file.close()
        if os.path.exists(file_name):
            os.remove(file_name)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_bad_json(self, arg_parse, ipc_channel):
        client = OdinDataClient()

        tmp_file = tempfile.NamedTemporaryFile(
            mode="w",
            buffering=-1,
            encoding=None,
            newline=None,
            suffix=None,
            prefix=None,
            dir=None,
            errors=None,
            delete=False,
        )
        #        tmp_file.write('{"test": "val"}\n')
        file_name = tmp_file.name
        tmp_file.close()
        r_file = open(file_name, "r")
        client.args.config_file = r_file
        client.args.request_config = False
        client.args.request_commands = False
        client.args.status = False
        client.args.shutdown = False

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_not_called()

        r_file.close()
        if os.path.exists(file_name):
            os.remove(file_name)

    @patch("odin_data.client.OdinDataClient._parse_arguments")
    def test_await_response(self, arg_parse, ipc_channel):
        client = OdinDataClient()

        client.ctrl_channel.poll = MagicMock()
        client.ctrl_channel.poll.return_value = ipc_channel.POLLIN
        client.ctrl_channel.recv = MagicMock()
        client.ctrl_channel.recv.return_value = '{"id": 1}'
        client.await_response()

    def test_parse(self, ipc_channel):
        with patch(
            "sys.argv",
            [
                "client.py",
                "--ctrl",
                "tcp://1.1.1.1:9999",
                "--request-config",
                "--request-commands",
                "--status",
                "--shutdown",
            ],
        ):
            # Verify command line arguments are parsed as expected
            client = OdinDataClient()
            self.assertEquals(client.args.ctrl_endpoint, "tcp://1.1.1.1:9999")
            self.assertEquals(client.args.request_config, True)
            self.assertEquals(client.args.request_commands, True)
            self.assertEquals(client.args.status, True)
            self.assertEquals(client.args.shutdown, True)
