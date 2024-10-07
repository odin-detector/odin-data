from odin_data.control.odin_data_adapter import OdinDataAdapter
from odin_data.control.odin_data_controller import OdinDataController


FrameReceiverController = OdinDataController


class FrameReceiverAdapter(OdinDataAdapter):
    _controller_cls = FrameReceiverController
