# FrameProcesssor Plugins

## FrameProcessorPlugin

Base class.

```{doxygenclass} FrameProcessor::FrameProcessorPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## BloscPlugin

Perform Various compression algorithms via the blosc library.

```{doxygenclass} FrameProcessor::BloscPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## GapFillPlugin

Insert gaps in Frame data to account for physical sensor spacing.

```{doxygenclass} FrameProcessor::GapFillPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## LiveViewPlugin

Publish Frame data on a ZeroMQ stream for live monitoring.

```{doxygenclass} FrameProcessor::LiveViewPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## OffsetAdjustmentPlugin

Modify the offset on a Frame to change where it appears in a file.

```{doxygenclass} FrameProcessor::OffsetAdjustmentPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## ParameterAdjustmentPlugin

Modify or create parameters on a Frame so that they can be written to a file.

```{doxygenclass} FrameProcessor::ParameterAdjustmentPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## SumPlugin

Calculate the sum of Frame data and add the result to the Frame.

```{doxygenclass} FrameProcessor::SumPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```

## FileWriterPlugin

Write Frame data to hdf5 files.

```{doxygenclass} FrameProcessor::FileWriterPlugin
   :members:
   :protected-members:
   :private-members:
   :undoc-members:
```
