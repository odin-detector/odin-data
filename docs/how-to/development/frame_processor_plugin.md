# FrameProcessorPlugin

The `FrameProcessorPlugin` is implemented for various purposes and an arbitrary tree of
plugins can be loaded and connected within the frameProcessor application. Detector
plugins are not special in any way from more general plugins, except that they are
generally the first plugin in the tree.

This class defines a few virtual methods to be implemented by concrete implementations.
The only strictly required method is `process_frame`

```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::process_frame
```

----------------------------------------------------------------------------------------

`configure`, `requestConfiguration` and `status` are technically optional, but they
should be implemented if the plugin has parameters to expose

```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::configure
```
```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::requestConfiguration
```
```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::status
```

----------------------------------------------------------------------------------------

If the plugin should take some action when an end of acquisition frame is passed, then
`process_end_of_acquisition` can be implemented to handle this case

```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::process_end_of_acquisition
```

----------------------------------------------------------------------------------------

Additionally, the base class provides the `push` method, which should be called by child
classes at the end of `process_frame` to pass frames on to any listening plugins in the
plugin tree. If the plugin is intended to only be at the end of a plugin chain, then
this does not need to be implemented.

```{doxygenfunction} FrameProcessor::FrameProcessorPlugin::push(boost::shared_ptr<Frame> frame)
```

## Types of Frame

The [Frame](../../reference/api/frame_processor/frame.md) class is the
container used to pass data between plugins. It is an abstract base class with a few
concrete implementations.

### SharedBufferFrame

The [SharedBufferFrame](../../reference/api/frame_processor/frame.md#sharedbufferframe)
simply stores the pointer to the shared memory buffer as provided by the frameReceiver.
When possible to perform any processing in place, it is best to publish an instance of
this class to subsequent plugins to avoid an unecessary copy.

### DataBlockFrame

For cases where it is not possible to operate in place a
[DataBlockFrame](../../reference/api/frame_processor/frame.md#datablockframe)
can be allocated from the DataBlockPool. For example, the BloscPlugin does this because
the compression algorithm requires input and output data pointers.
