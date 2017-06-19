# Install header files into installation prefix

SET(HEADERS ClassLoader.h gettime.h IpcChannel.h IpcMessage.h IpcReactor.h OdinDataException.h SharedBufferManager.h)
SET(RAPIDJSON_INCLUDE_DIR rapidjson)
SET(ZMQ_INCLUDE_DIR zmq)

INSTALL(FILES ${HEADERS} DESTINATION include)
INSTALL(DIRECTORY ${RAPIDJSON_INCLUDE_DIR} DESTINATION include)
INSTALL(DIRECTORY ${ZMQ_INCLUDE_DIR} DESTINATION include)