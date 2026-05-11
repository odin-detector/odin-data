import json
import time
from unittest import TestCase
from unittest.mock import DEFAULT, MagicMock, patch

import zmq
from odin_data.control.ipc_client import IpcClient, IpcMessageException
from odin_data.control.odin_data_controller import IpcTornadoClient, OdinDataController


@patch("odin_data.control.odin_data_controller.IpcTornadoClient")
class TestOdinDataController(TestCase):
    def test_configuration(self, ipcClient):

        controller = OdinDataController("testname", "127.0.0.1:10000")

        # Assert IpcChannel constructed
        ipcClient.assert_called_once()

        # Get the controller API and assert return
        api = controller.get("api", True)
        self.assertAlmostEqual(api["api"]["value"], 0.1)
        self.assertEqual(api["api"]["writeable"], False)
        self.assertEqual(api["api"]["type"], "float")

        # Put a simple value to config
        controller.put("0/config", {})

        # Set an error
        controller.set_error("Test error 1")
        # Read the error and assert it has been set
        err = controller.get("0/status/error", True)
        self.assertEqual(err["error"]["value"], "Test error 1")

        # Clear the error and reassert it is empty
        controller.clear_error()
        err = controller.get("0/status/error", True)
        self.assertEqual(err["error"]["value"], "")

        # Patch the ipcClient to return true for connected status
        controller._clients[0].connected.return_value = True
        controller._clients[0].parameters = {"status": {}, "config": {}, "commands": {}}

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
        self.assertAlmostEquals(cfg, {"p1": {"p2": []}})

        # Set up a difference in the config cache
        controller._clients[0].send_configuration = MagicMock()

        # Call the merge external method
        controller.merge_external_tree("0", {"config": {"item1": "Value1"}})
        controller._config_cache = [{"config": {"item1": "Value2"}}]
        controller.process_config_changes()

        # Assert the difference is sent as a configuration command
        controller._clients[0].send_configuration.assert_called_once_with(
            {"item1": "Value1"}
        )

        # Allow time for the update loop to execute once
        time.sleep(1.0)

        controller.shutdown()
