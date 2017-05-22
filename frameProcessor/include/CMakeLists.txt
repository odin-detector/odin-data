# Install header files into installation prefix

SET(HEADERS DataBlock.h DataBlockPool.h Frame.h FrameProcessorPlugin.h IFrameCallback.h MetaMessage.h WorkQueue.h)
INSTALL(FILES ${HEADERS} DESTINATION include/frameProcessor)
