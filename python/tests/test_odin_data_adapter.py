import json
import time
from unittest import TestCase
from unittest.mock import MagicMock, patch

import zmq
from odin.adapters.adapter import ApiAdapterRequest
from odin.adapters.parameter_tree import (
    ParameterTreeError,
)
from odin_data.control.odin_data_adapter import OdinDataAdapter, OdinDataController


@patch("odin_data.control.odin_data_adapter.OdinDataAdapter._controller_cls")
class TestOdinDataAdapter(TestCase):
    def test_no_endpoints(self, controller_cls):
        def create_adapter():
            OdinDataAdapter()

        self.assertRaises(RuntimeError, create_adapter)

    def test_build_and_tear_down(self, controller_cls):
        adapter = OdinDataAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        self.assertAlmostEqual(adapter._update_interval, 1.0)
        controller = adapter._controller

        # Assert controller has been constructed
        self.assertIsInstance(controller, MagicMock)

        # Assert shutdown called on adapter cleanup
        controller.shutdown = MagicMock()
        adapter.cleanup()
        controller.shutdown.assert_called_once()

    def test_get(self, controller_cls):
        adapter = OdinDataAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        controller = adapter._controller
        controller.get = MagicMock()
        req = ApiAdapterRequest(data={0.5})
        adapter.get("test/item", req)

        # Set the get method to return error response
        def get_response_raise(path, req):
            raise ParameterTreeError("Test error")

        controller.get.side_effect = get_response_raise

        # Call adapter get and assert the response
        resp = adapter.get("test/item", req)
        self.assertEqual(resp.status_code, 400)

    def test_put(self, controller_cls):
        adapter = OdinDataAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        controller = adapter._controller
        controller.put = MagicMock()
        req = ApiAdapterRequest(data="1")
        resp = adapter.put("test/item", req)

        # Assert the path and request body are sent to the controller
        controller.put.assert_called_once_with("test/item", 1)
        # Assert the response contains a good status code
        self.assertEqual(resp.status_code, 200)

        # Set the put method to return error response
        def set_response_raise(path, req):
            raise ParameterTreeError("Test error")

        controller.put.side_effect = set_response_raise

        # Call adapter put and assert the response
        resp = adapter.put("test/item", req)
        self.assertEqual(resp.status_code, 400)
