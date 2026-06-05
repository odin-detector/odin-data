import pytest
from unittest.mock import MagicMock, patch

from odin.adapters.adapter import ApiAdapterRequest
from odin.adapters.parameter_tree import ParameterTreeError
from odin_data.control.odin_data_adapter import OdinDataAdapter


@pytest.fixture
def controller_cls():
    with patch("odin_data.control.odin_data_adapter.OdinDataAdapter._controller_cls") as mock:
        yield mock


@pytest.fixture
def adapter(controller_cls):
    return OdinDataAdapter(endpoints="127.0.0.1:10000", update_interval=1.0)


class TestOdinDataAdapter:
    def test_no_endpoints(self, controller_cls):
        with pytest.raises(RuntimeError):
            OdinDataAdapter()

    def test_build_and_tear_down(self, adapter):
        assert adapter._update_interval == pytest.approx(1.0)
        controller = adapter._controller

        assert isinstance(controller, MagicMock)

        controller.shutdown = MagicMock()
        adapter.cleanup()
        controller.shutdown.assert_called_once()

    def test_get(self, adapter):
        controller = adapter._controller
        controller.get = MagicMock()
        req = ApiAdapterRequest(data={0.5})
        adapter.get("test/item", req)

        def get_response_raise(path, req):
            raise ParameterTreeError("Test error")

        controller.get.side_effect = get_response_raise

        resp = adapter.get("test/item", req)
        assert resp.status_code == 400

    def test_put(self, adapter):
        controller = adapter._controller
        controller.put = MagicMock()
        req = ApiAdapterRequest(data="1")
        resp = adapter.put("test/item", req)

        controller.put.assert_called_once_with("test/item", 1)
        assert resp.status_code == 200

        def set_response_raise(path, req):
            raise ParameterTreeError("Test error")

        controller.put.side_effect = set_response_raise

        resp = adapter.put("test/item", req)
        assert resp.status_code == 400
