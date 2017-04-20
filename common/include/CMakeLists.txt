# Install header files
SET(HEADERS ClassLoader.h gettime.h IpcChannel.h IpcMessage.h IpcReactor.h OdinDataException.h SharedBufferManager.h)
INSTALL(FILES ${HEADERS} DESTINATION ${CMAKE_BINARY_DIR}/include)
