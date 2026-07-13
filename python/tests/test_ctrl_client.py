import os
import tempfile
from unittest.mock import MagicMock, patch

import pytest
from odin_data.client import OdinDataClient


@pytest.fixture
def ipc_channel():
    with patch("odin_data.client.IpcChannel") as mock:
        yield mock


@pytest.fixture
def client(ipc_channel):
    with patch("odin_data.client.OdinDataClient._parse_arguments"):
        client = OdinDataClient()
        client.args.config_file = None
        client.args.request_config = False
        client.args.request_commands = False
        client.args.status = False
        client.args.shutdown = False
        yield client


class TestClientProgram:
    def test_client(self, client):
        assert client._msg_id == 0
        new_id = client._next_msg_id()
        assert new_id == 1
        assert client._msg_id == 1

    def test_shutdown(self, client):
        client.args.shutdown = True

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        assert '"id": 1' in args[0]
        assert '"msg_type": "cmd"' in args[0]
        assert '"msg_val": "shutdown"' in args[0]

    def test_status(self, client):
        client.args.status = True

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        assert '"id": 1' in args[0]
        assert '"msg_type": "cmd"' in args[0]
        assert '"msg_val": "status"' in args[0]

    def test_request_command(self, client):
        client.args.request_commands = True

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        assert '"id": 1' in args[0]
        assert '"msg_type": "cmd"' in args[0]
        assert '"msg_val": "request_commands"' in args[0]

    def test_request_config(self, client):
        client.args.request_config = True

        client.ctrl_channel.send = MagicMock()
        client.run()
        client.ctrl_channel.send.assert_called_once()
        args, kwargs = client.ctrl_channel.send.call_args
        assert '"id": 1' in args[0]
        assert '"msg_type": "cmd"' in args[0]
        assert '"msg_val": "request_configuration"' in args[0]

    def test_config_cmd(self, client, tmp_path):
        file = tmp_path / "config.json"
        file.write_text('{"test": "val"}\n')
        file_name = file.absolute()

        with open(file_name, "r") as r_file:
            client.args.config_file = r_file

            client.ctrl_channel.send = MagicMock()
            client.run()
            client.ctrl_channel.send.assert_called_once()
            args, kwargs = client.ctrl_channel.send.call_args
            assert '"id": 1' in args[0]
            assert '"msg_type": "cmd"' in args[0]
            assert '"msg_val": "configure"' in args[0]
            assert '"params": {"test": "val"}' in args[0]

    def test_bad_json(self, client, tmp_path):
        file = tmp_path / "config.json"
        file.write_text("")
        file_name = file.absolute()

        with open(file_name, "r") as r_file:
            client.args.config_file = r_file

            client.ctrl_channel.send = MagicMock()
            client.run()
            client.ctrl_channel.send.assert_not_called()

    def test_await_response(self, client, ipc_channel):
        client.ctrl_channel.poll = MagicMock(return_value=ipc_channel.POLLIN)
        client.ctrl_channel.recv = MagicMock(return_value='{"id": 1}')
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
            client = OdinDataClient()
            assert client.args.ctrl_endpoint == "tcp://1.1.1.1:9999"
            assert client.args.request_config is True
            assert client.args.request_commands is True
            assert client.args.status is True
            assert client.args.shutdown is True
