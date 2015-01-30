
set(CMAKE_INCLUDE_CURRENT_DIR on)

include_directories(${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

add_executable(frameReceiver appMain.cpp FrameReceiverApp.cpp)

target_link_libraries(frameReceiver ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
