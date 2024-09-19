from odin_data.control.odin_data_adapter import OdinDataAdapter
from odin_data.control.frame_receiver_controller import FrameReceiverController


class FrameReceiverAdapter(OdinDataAdapter):
    _controller_cls = FrameReceiverController
