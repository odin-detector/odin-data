from odin_data.control.odin_data_adapter import OdinDataAdapter
from odin_data.control.frame_processor_controller import FrameProcessorController


class FrameProcessorAdapter(OdinDataAdapter):
    _controller_cls = FrameProcessorController
