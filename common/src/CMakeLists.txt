# Add library for IPC classes
add_library(Ipc SHARED IpcChannel.cpp IpcMessage.cpp IpcReactor.cpp)
target_link_libraries(Ipc ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
