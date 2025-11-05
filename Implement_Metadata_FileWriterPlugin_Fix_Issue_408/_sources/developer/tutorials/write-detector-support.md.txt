# Writing Detector Support

Creating odin-data support for a detector requires creating a [FrameDecoder] and a
[FrameProcessorPlugin]. Optionally, a [FrameSimulatorPlugin] can be created, which may be
useful for testing and development.


## Existing Detector Support

The following modules implement data acquisition using odin-data:

-  [Excalibur](https://github.com/dls-controls/excalibur-detector)
-  [Eiger](https://github.com/dls-controls/eiger-detector)
-  [Tristan - LATRD](https://github.com/dls-controls/tristan-detector)
-  [Percival](https://github.com/percival-detector/percival-detector)

[FrameDecoder]: ../how-to/frame-decoder
[FrameProcessorPlugin]: ../how-to/frame-processor-plugin
[FrameSimulatorPlugin]: ../how-to/frame-simulator-plugin
