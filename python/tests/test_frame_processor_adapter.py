from unittest.mock import patch

from odin_data.control.frame_processor_adapter import FrameProcessorAdapter
from odin_data.control.frame_processor_controller import FrameProcessorController


class TestFrameProcessorAdapter:
    def test_adapter_creation(self):
        with patch.object(
            FrameProcessorAdapter, "__init__", return_value=None
        ) as mock_method:
            adapter = FrameProcessorAdapter()
            assert adapter._controller_cls == FrameProcessorController
            mock_method.assert_called_once()
