import json
import time
from unittest import TestCase
from unittest.mock import MagicMock, patch

import zmq
from odin.adapters.adapter import ApiAdapterRequest
from odin.adapters.parameter_tree import (
    ParameterTreeError,
)
from odin_data.control.meta_listener_adapter import MetaListenerAdapter
from odin_data.control.odin_data_adapter import OdinDataAdapter


@patch("odin_data.control.odin_data_adapter.OdinDataAdapter._controller_cls")
class TestMetaListenerAdapter(TestCase):
    def test_get(self, controller_cls):
        adapter = MetaListenerAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        # Setup internal values ready to be verified with get calls
        adapter.acquisition_id = 1
        adapter.acquisition_active = True

        req = ApiAdapterRequest(data=None, content_type="application/json")
        resp = adapter.get("config/acquisition_id", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["acquisition_id"], 1)

        resp = adapter.get("status/acquisition_active", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["acquisition_active"], True)

        adapter._client.parameters = {}
        resp = adapter.get("config/acquisitions", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["acquisitions"], None)

        adapter._client.parameters = {
            "config": {"acquisitions": {"acqone": None, "acqtwo": None}}
        }
        resp = adapter.get("config/acquisitions", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["acquisitions"], "acqone,acqtwo")

        adapter._status_parameters["test/path/status1"] = "value1"
        resp = adapter.get("test/path/status1", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["status1"], "value1")

        adapter._config_parameters["test/path/config2"] = "value2"
        resp = adapter.get("test/path/config2", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.data["config2"], "value2")

        resp = adapter.get("some/other/path", req)
        self.assertEqual(resp.status_code, 200)
        adapter._controller.get.assert_called_once()

    def test_put(self, controller_cls):
        adapter = MetaListenerAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        # Setup internal values ready to be verified with get calls
        adapter.acquisition_id = 1
        adapter.acquisition_active = True
        adapter._client.parameters = {
            "config": {"acquisitions": {"acqone": None, "acqtwo": None}}
        }
        adapter._status_parameters["test/path/status1"] = "value1"
        adapter._config_parameters["config/test"] = "value2"

        req = ApiAdapterRequest(data="value3", content_type="application/json")
        resp = adapter.put("config/test", req)
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(adapter._config_parameters["config/test"], "value3")

        adapter._client = MagicMock()
        req = ApiAdapterRequest(data="Acq2", content_type="application/json")
        resp = adapter.put("config/acquisition_id", req)
        self.assertEqual(resp.status_code, 200)
        adapter._client.send_configuration.assert_called_once_with(
            {
                "acquisition_id": "Acq2",
                "directory": "",
                "file_prefix": "",
                "flush_frame_frequency": 100,
                "flush_timeout": 1,
                "test": "value3",
            }
        )

        # Set the get method to return error response
        def send_response_raise(config):
            raise Exception("Test error")

        adapter._client.send_configuration.side_effect = send_response_raise
        req = ApiAdapterRequest(data="Acq2", content_type="application/json")
        resp = adapter.put("config/acquisition_id", req)
        self.assertEqual(resp.status_code, 503)

        adapter._client = MagicMock()
        req = ApiAdapterRequest(data="", content_type="application/json")
        resp = adapter.put("config/stop", req)
        self.assertEqual(resp.status_code, 200)
        adapter._client.send_configuration.assert_called_once_with(
            {
                "acquisition_id": "Acq2",
                "stop": True,
            }
        )

        # Test any other items are passed through to the underlying adapter
        adapter.acquisition_id = None
        req = ApiAdapterRequest(data='{"item":"Test"}', content_type="application/json")
        resp = adapter.put("other", req)
        self.assertEqual(resp.status_code, 200)

    def test_process(self, controller_cls):
        adapter = MetaListenerAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)

        # Verify the relevant acqusition parameters are reported as status items
        adapter.acquisition_id = "acqone"
        adapter.acquisition_active = True
        adapter._client.parameters = {
            "status": {"acquisitions": {"acqone": {"full_file_path": "/test"}}},
        }
        adapter.process_updates()
        self.assertEqual(adapter._status_parameters["status/full_file_path"], "/test")

        # Set the active flag to false
        adapter.acquisition_active = False
        adapter._client.parameters = {
            "status": {"acquisitions": None},
        }
        adapter.process_updates()
        # Verify defaults are now reported as status
        self.assertEqual(adapter._status_parameters["status/full_file_path"], "")

        # Set the acquisition ID to None
        adapter.acquisition_id = None
        adapter.process_updates()
        # Verify defaults are reported as status
        self.assertEqual(adapter._status_parameters["status/full_file_path"], "")
