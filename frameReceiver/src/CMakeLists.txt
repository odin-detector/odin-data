
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMERECEIVER_DIR}/include ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

file(GLOB LIB_SOURCES FrameDecoder.cpp)

# Add library for common plugin code
add_library(${LIB_RECEIVER} SHARED ${LIB_SOURCES})
target_link_libraries(${LIB_RECEIVER} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS ${LIB_RECEIVER} DESTINATION lib)

file(GLOB APP_SOURCES FrameReceiverApp.cpp
                      FrameReceiverController.cpp
                      FrameReceiverRxThread.cpp
                      FrameReceiverUDPRxThread.cpp
                      FrameReceiverZMQRxThread.cpp )

add_executable(frameReceiver ${APP_SOURCES})

target_link_libraries(frameReceiver ${LIB_RECEIVER} ${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    find_library(REALTIME_LIBRARY
             NAMES rt)
    find_library(PTHREAD_LIBRARY
             NAMES pthread)
    target_link_libraries(frameReceiver ${PTHREAD_LIBRARY} ${REALTIME_LIBRARY})
endif()

install(TARGETS frameReceiver RUNTIME DESTINATION bin)

add_library(DummyUDPFrameDecoder SHARED DummyUDPFrameDecoder.cpp DummyUDPFrameDecoderLib.cpp)
target_link_libraries(DummyUDPFrameDecoder ${LIB_RECEIVER} ${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
install(TARGETS DummyUDPFrameDecoder DESTINATION lib)
