import json
import time
from unittest import TestCase
from unittest.mock import DEFAULT, patch

import zmq
from odin_data.control.ipc_client import IpcClient, IpcMessageException
from odin_data.control.odin_data_controller import IpcTornadoClient, OdinDataController


@patch("odin_data.control.odin_data_controller.IpcTornadoClient")
class TestOdinDataController(TestCase):
    def test_send_request(self, ipcClient):

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

        controller.create_demand_config(
            {
                "p1": {"p2": ["p3", "p4"]},
                "p5": True,
            },
            {
                "p1": {"p2": ["p3", "p4"]},
            },
        )

        # Allow time for the update loop to execute once
        time.sleep(1.0)

        controller.shutdown()
