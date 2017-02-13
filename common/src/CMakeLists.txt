# Add library for odin-data shared functionality

add_library(${COMMON_LIBRARY} SHARED IpcChannel.cpp IpcMessage.cpp IpcReactor.cpp)
target_link_libraries(${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
