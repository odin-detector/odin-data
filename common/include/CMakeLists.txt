# Install header files
SET(HEADERS ClassLoader.h gettime.h IpcChannel.h IpcMessage.h IpcReactor.h OdinDataException.h SharedBufferManager.h)
INSTALL(FILES ${HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include)

SET(RAPIDJSON_INCLUDE_DIR rapidjson)
message("RAPIDJSON_INCLUDE_DIR ${RAPIDJSON_INCLUDE_DIR}")
INSTALL(DIRECTORY ${RAPIDJSON_INCLUDE_DIR} DESTINATION ${CMAKE_BINARY_DIR}/include)
