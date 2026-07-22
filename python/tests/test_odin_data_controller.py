import time
from unittest.mock import MagicMock, Mock, patch

import pytest
from odin_data.control.odin_data_controller import (
    OdinDataController,
    getter_func,
    setter_func,
)


@pytest.fixture(scope="module")
def controller():
    with patch(
        "odin_data.control.odin_data_controller.IpcTornadoClient"
    ) as IpcTornadoClient_mock:
        IpcTornadoClient_mock.IPC_VAL_STATUS = "status"
        IpcTornadoClient_mock.STATUS_PARAMS_KEY = "status_request"
        IpcTornadoClient_mock.IPC_VAL_STATUS_METADATA = "status_metadata"
        IpcTornadoClient_mock.IPC_VAL_STATUS_METADATA_HASH = "status_metadata_hash"
        IpcTornadoClient_mock.IPC_VAL_CONFIG = "config"
        IpcTornadoClient_mock.CONFIG_PARAMS_KEY = "config_request"
        IpcTornadoClient_mock.IPC_VAL_CONFIG_METADATA = "config_metadata"
        IpcTornadoClient_mock.IPC_VAL_CONFIG_METADATA_HASH = "config_metadata_hash"
        IpcTornadoClient_mock.return_value.parameters = {
            "config": {
                "config_request": {
                    "blosc": {"compressor": "lz4", "level": 1, "shuffle": "shuffle", "threads": 1},
                    "live": {"compressor": "lz4", "level": 1, "shuffle": "shuffle", "threads": 1},
                },
                "config_metadata_hash": 1234567,
                "config_metadata": {
                    "blosc": {
                        "compressor": {
                            "access_mode": "rw",
                            "allowed_values": ["blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd"],
                            "type": "string",
                        },
                        "level": {
                            "access_mode": "rw",
                            "max": 9,
                            "min": 0,
                            "type": "int",
                        },
                        "shuffle": {
                            "access_mode": "rw",
                            "allowed_values": ["noshuffle", "shuffle", "bitshuffle"],
                            "type": "string",
                        },
                        "threads": {"access_mode": "rw", "min": 1, "type": "uint"},
                    },
                    "live": {
                        "compressor": {
                            "access_mode": "rw",
                            "allowed_values": ["blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd"],
                            "type": "string",
                        },
                        "level": {
                            "access_mode": "rw",
                            "max": 9,
                            "min": 0,
                            "type": "int",
                        },
                        "shuffle": {
                            "access_mode": "rw",
                            "allowed_values": [0, 1, 2],
                            "type": "uint",
                        },
                        "threads": {"access_mode": "rw", "min": 1, "type": "uint"},
                    },
                },
                "params": {
                    "id": 1,
                    "msg_type": "ack",
                    "msg_val": "request_configuration",
                    "timestamp": "2026-03-06T16:53:41.165801",
                    "dummy": {"copy_frame": True, "height": 1024, "width": 1400},
                    "fr_setup": {
                        "fr_ready_cnxn": "tcp://127.0.0.1:10001",
                        "fr_release_cnxn": "tcp://127.0.0.1:10002",
                    },
                    "meta_endpoint": "tcp://*:10008",
                },
            }
        }
        controller = OdinDataController("testname", "127.0.0.1:10000")
        yield controller

        controller.shutdown()


class TestOdinDataController:
    # controller() method passed to this test.
    def test_configuration(self, controller):

        # Ensure the client doesn't timeout
        wait_for_response_mock = MagicMock()
        wait_for_response_mock.return_value = False
        controller._clients[0].wait_for_response = wait_for_response_mock

        # Get the controller API and assert return
        api = controller.get("api", True)
        assert api["api"]["value"] == 0.1
        assert not api["api"]["writeable"]
        assert api["api"]["type"] == "float"

        # Put a simple value to config
        controller.put("0/config", {})

        # Set an error
        controller.set_error("Test error 1")
        # Read the error and assert it has been set
        err = controller.get("0/status/error", True)
        assert err["error"]["value"], "Test error 1"

        # Clear the error and reassert it is empty
        controller.clear_error()
        err = controller.get("0/status/error", True)
        assert err["error"]["value"] == ""

        # Patch the ipcClient to return true for connected status
        controller._clients[0].connected.return_value = True
        # Call creation of config on two mismatched dicts
        cfg = controller.create_demand_config(
            {
                "p1": {"p2": ["p3", "p4"]},
                "p5": True,
            },
            {
                "p1": {"p2": ["p3", "p4"]},
            },
        )
        # Verify the reply contains the matched structure of the two dicts
        assert cfg == {"p1": {"p2": []}}

        # Allow time for the update_loop() to execute once
        time.sleep(1.0)

        ################TEST THE CONFIGURATION#################
        # Mock the setter function for 'compressor'
        controller._params._tree["0"]["config"]["blosc"]["compressor"]._set = MagicMock()
        # make a put request as a typical client would
        controller.put("0/config/blosc/compressor", "lz4hc")
        # Assert that the '_set' function in the ParameterTree is called!
        controller._params._tree["0"]["config"]["blosc"]["compressor"]._set.assert_called_once_with("lz4hc")

        # Set a new value for the 'compressor'
        controller._config_resposes[0]["blosc"]["compressor"] = "snappy"
        compressor = controller.get("0/config/blosc/compressor", True)
        assert compressor == {
            "compressor": {
                "value": "snappy",
                "allowed_values": ["blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd"],
                "writeable": True,
                "type": "str",
            }
        }
