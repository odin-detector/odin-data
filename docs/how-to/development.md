# Developer Guide

Creating odin-data support for a detector requires creating a
[FrameDecoder](development/frame_decoder) and a
[FrameProcessorPlugin](development/frame_processor_plugin). Optionally, a
[FrameSimulatorPlugin](development/frame_simulator_plugin) can be created,
which may be useful for testing and development.


## Existing Detector Support

The following modules implement data acquisition using odin-data:

-  [Excalibur](https://github.com/dls-controls/excalibur-detector)
-  [Eiger](https://github.com/dls-controls/eiger-detector)
-  [Tristan - LATRD](https://github.com/dls-controls/tristan-detector)
-  [Percival](https://github.com/percival-detector/percival-detector)
