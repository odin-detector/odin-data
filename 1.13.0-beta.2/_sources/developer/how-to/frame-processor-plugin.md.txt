# FrameProcessorPlugin

The [FrameProcessorPlugin] is implemented for various purposes and an arbitrary tree of
plugins can be loaded and connected within the frameProcessor application. Detector
plugins are not special in any way from more general plugins, except that they are
generally the first plugin in the tree.

This class defines a few virtual methods to be implemented by concrete implementations.

- [process_frame]
- [configure]*
- [requestConfiguration]*
- [status]*
- [process_end_of_acquisition]*

While the [configure], [requestConfiguration] and [status] methods are technically
optional, they should be implemented if the plugin has parameters to expose.

If the plugin should take some action when an
[end of acquisition frame](#endofacquisitionframe) is passed, then
[process_end_of_acquisition] can be implemented to handle this case.

Additionally, the base class provides the [push] method, which should be called by child
classes at the end of [process_frame] to pass frames on to  any listening plugins in the
plugin tree. If the plugin is intended to only be at the end of a plugin chain, then this
does not need to be implemented.

## Types of Frame

The [Frame] class is the container used to pass data between plugins. It is an abstract
base class with a few concrete implementations.

### SharedBufferFrame

The [SharedBufferFrame] simply stores the pointer to the shared memory buffer as provided
by the frameReceiver. When possible to perform any processing in place, it is best to
publish an instance of this class to subsequent plugins to avoid an unecessary copy.

### DataBlockFrame

For cases where it is not possible to operate in place a [DataBlockFrame] can be
allocated from the DataBlockPool. For example, the BloscPlugin does this because the
compression algorithm requires input and output data pointers.

### EndOfAcquisitionFrame

The [EndOfAcquisitionFrame] is a meta Frame that is used only to signal the end of an
acquisition to plugins further down the plugin chain. This is typically produced by
detector processing plugins that can detect the end of acquisition from the data stream.

[FrameProcessorPlugin]: FrameProcessor::FrameProcessorPlugin
[process_frame]: FrameProcessor::FrameProcessorPlugin::process_frame
[configure]: FrameProcessor::FrameProcessorPlugin::configure
[requestConfiguration]: FrameProcessor::FrameProcessorPlugin::requestConfiguration
[status]: FrameProcessor::FrameProcessorPlugin::status
[process_end_of_acquisition]: FrameProcessor::FrameProcessorPlugin::process_end_of_acquisition
[push]: FrameProcessor::FrameProcessorPlugin::push
[Frame]: FrameProcessor::Frame
[SharedBufferFrame]: FrameProcessor::SharedBufferFrame
[DataBlockFrame]: FrameProcessor::DataBlockFrame
[EndOfAcquisitionFrame]: FrameProcessor::EndOfAcquisitionFrame
