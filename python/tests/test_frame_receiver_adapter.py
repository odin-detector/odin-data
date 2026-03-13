from unittest.mock import patch

from odin_data.control.frame_receiver_adapter import FrameReceiverAdapter
from odin_data.control.frame_receiver_controller import FrameReceiverController


class TestFrameReceiverAdapter:
    def test_adapter_creation(self):
        with patch.object(
            FrameReceiverAdapter, "__init__", return_value=None
        ) as mock_method:
            adapter = FrameReceiverAdapter()
            assert adapter._controller_cls == FrameReceiverController
            mock_method.assert_called_once()
