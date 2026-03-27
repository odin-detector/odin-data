# Function to add a FrameReceiver unit test. The source must be of the form
#  <target_name>UnitTest.cpp
# The target name will be prefixed with unit_fr_ to differentiate from any other type of
# unit test executables.
function(add_unit_test _target)
  set(_source ${_target}UnitTest.cpp)
  set(_target "unit_fr_${_target}")
  add_test(NAME ${_target} COMMAND ${_target} --log_level=test_suite)
  set_tests_properties(${_target} PROPERTIES LABELS "frameReceiver")

  # For some reason the receiver lib doesn't contain many of the FrameReceiver classes so they
  # have to be built here.
  # TODO Would it be better to build those into the receiver lib?
  set(RECEIVER_SRCS
    ${FRAMERECEIVER_DIR}/src/FrameReceiverRxThread.cpp
    ${FRAMERECEIVER_DIR}/src/FrameReceiverTCPRxThread.cpp
    ${FRAMERECEIVER_DIR}/src/FrameReceiverUDPRxThread.cpp
    ${FRAMERECEIVER_DIR}/src/FrameReceiverZMQRxThread.cpp
  )

  add_executable(${_target} ${_source} ${RECEIVER_SRCS})

  target_link_libraries(${_target}
    ${LIB_RECEIVER}
    DummyTCPFrameDecoder
    DummyUDPFrameDecoder
    ${Boost_LIBRARIES}
    ${LOG4CXX_LIBRARIES}
    ${ZEROMQ_LIBRARIES}
  )

  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    # librt required for timing functions
    find_library(REALTIME_LIBRARY NAMES rt)
    find_library(PTHREAD_LIBRARY NAMES pthread)
    target_link_libraries(${_target} ${REALTIME_LIBRARY} ${PTHREAD_LIBRARY})
  endif()

  install(TARGETS ${_target} RUNTIME DESTINATION bin)
endfunction()

set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(
  ${FRAMERECEIVER_DIR}/include
  ${Boost_INCLUDE_DIRS}
  ${LOG4CXX_INCLUDE_DIRS}/..
  ${ZEROMQ_INCLUDE_DIRS}
)

add_unit_test(FrameReceiverConfig)
add_unit_test(FrameReceiverRxThread)
add_unit_test(IpcChannel)
add_unit_test(IpcMessage)
add_unit_test(IpcReactor)
add_unit_test(SharedBufferManager)
