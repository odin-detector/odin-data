
set(CMAKE_INCLUDE_CURRENT_DIR on)

include_directories(${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

file(GLOB APP_SOURCES *.cpp)

add_executable(frameReceiver ${APP_SOURCES})

target_link_libraries(frameReceiver ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})

# Add library for IPC classes
add_library(Ipc SHARED IpcChannel.cpp IpcMessage.cpp IpcReactor.cpp)
target_link_libraries(Ipc ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
