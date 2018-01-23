# Install header files into installation prefix

SET(HEADERS DataBlock.h DataBlockPool.h Frame.h FrameProcessorPlugin.h FileWriterPlugin.h BloscPlugin.h IFrameCallback.h MetaMessage.h MetaMessagePublisher.h WorkQueue.h HDF5File.h Acquisition.h FrameProcessorDefinitions.h)
INSTALL(FILES ${HEADERS} DESTINATION include/frameProcessor)
