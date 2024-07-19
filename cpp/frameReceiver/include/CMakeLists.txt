# Install header files into installation prefix

SET(HEADERS FrameDecoder.h FrameDecoderUDP.h FrameDecoderZMQ.h FrameDecoderTCP.h)
INSTALL(FILES ${HEADERS} DESTINATION include/frameReceiver)
