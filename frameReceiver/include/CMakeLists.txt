# Install header files
SET(HEADERS DebugLevelLogger.h FrameDecoder.h FrameDecoderUDP.h FrameDecoderZMQ.h)
INSTALL(FILES ${HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include)
