import pytest
from unittest.mock import MagicMock, patch

from odin.adapters.adapter import ApiAdapterRequest
from odin_data.control.meta_listener_adapter import MetaListenerAdapter


@pytest.fixture
def adapter():
    with patch("odin_data.control.odin_data_adapter.OdinDataAdapter._controller_cls"):
        yield MetaListenerAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)


class TestMetaListenerAdapter:
    def test_get(self, adapter):
        adapter.acquisition_id = 1
        adapter.acquisition_active = True

        req = ApiAdapterRequest(data=None, content_type="application/json")

        resp = adapter.get("config/acquisition_id", req)
        assert resp.status_code == 200
        assert resp.data["acquisition_id"] == 1

        resp = adapter.get("status/acquisition_active", req)
        assert resp.status_code == 200
        assert resp.data["acquisition_active"] is True

        adapter._client.parameters = {}
        resp = adapter.get("config/acquisitions", req)
        assert resp.status_code == 200
        assert resp.data["acquisitions"] is None

        adapter._client.parameters = {
            "config": {"acquisitions": {"acqone": None, "acqtwo": None}}
        }
        resp = adapter.get("config/acquisitions", req)
        assert resp.status_code == 200
        assert resp.data["acquisitions"] == "acqone,acqtwo"

        adapter._status_parameters["test/path/status1"] = "value1"
        resp = adapter.get("test/path/status1", req)
        assert resp.status_code == 200
        assert resp.data["status1"] == "value1"

        adapter._config_parameters["test/path/config2"] = "value2"
        resp = adapter.get("test/path/config2", req)
        assert resp.status_code == 200
        assert resp.data["config2"] == "value2"

        resp = adapter.get("some/other/path", req)
        assert resp.status_code == 200
        adapter._controller.get.assert_called_once()

    def test_put(self, adapter):
        adapter.acquisition_id = 1
        adapter.acquisition_active = True
        adapter._client.parameters = {
            "config": {"acquisitions": {"acqone": None, "acqtwo": None}}
        }
        adapter._status_parameters["test/path/status1"] = "value1"
        adapter._config_parameters["config/test"] = "value2"

        req = ApiAdapterRequest(data="value3", content_type="application/json")
        resp = adapter.put("config/test", req)
        assert resp.status_code == 200
        assert adapter._config_parameters["config/test"] == "value3"

        adapter._client = MagicMock()
        req = ApiAdapterRequest(data="Acq2", content_type="application/json")
        resp = adapter.put("config/acquisition_id", req)
        assert resp.status_code == 200
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

        def send_response_raise(config):
            raise Exception("Test error")

        adapter._client.send_configuration.side_effect = send_response_raise
        req = ApiAdapterRequest(data="Acq2", content_type="application/json")
        resp = adapter.put("config/acquisition_id", req)
        assert resp.status_code == 503

        adapter._client = MagicMock()
        req = ApiAdapterRequest(data="", content_type="application/json")
        resp = adapter.put("config/stop", req)
        assert resp.status_code == 200
        adapter._client.send_configuration.assert_called_once_with(
            {
                "acquisition_id": "Acq2",
                "stop": True,
            }
        )

        adapter.acquisition_id = None
        req = ApiAdapterRequest(data='{"item":"Test"}', content_type="application/json")
        resp = adapter.put("other", req)
        assert resp.status_code == 200

    def test_process(self, adapter):
        adapter.acquisition_id = "acqone"
        adapter.acquisition_active = True
        adapter._client.parameters = {
            "status": {"acquisitions": {"acqone": {"full_file_path": "/test"}}},
        }
        adapter.process_updates()
        assert adapter._status_parameters["status/full_file_path"] == "/test"

        adapter.acquisition_active = False
        adapter._client.parameters = {
            "status": {"acquisitions": None},
        }
        adapter.process_updates()
        assert adapter._status_parameters["status/full_file_path"] == ""

        adapter.acquisition_id = None
        adapter.process_updates()
        assert adapter._status_parameters["status/full_file_path"] == ""
