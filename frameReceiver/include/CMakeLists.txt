# Install header files into installation prefix

SET(HEADERS DebugLevelLogger.h FrameDecoder.h FrameDecoderUDP.h FrameDecoderZMQ.h)
INSTALL(FILES ${HEADERS} DESTINATION include)
