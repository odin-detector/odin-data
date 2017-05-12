
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMERECEIVER_DIR}/include ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

file(GLOB APP_SOURCES	appMain.cpp 
						FrameReceiverApp.cpp
						FrameReceiverRxThread.cpp
						FrameReceiverUDPRxThread.cpp
						FrameReceiverZMQRxThread.cpp)

add_executable(frameReceiver ${APP_SOURCES})

target_link_libraries(frameReceiver ${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})


