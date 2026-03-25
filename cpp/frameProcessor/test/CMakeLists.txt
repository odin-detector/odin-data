# Function to add a FrameProcessor unit test. The source must be of the form
#  <target_name>Test.cpp
# The target name will be prefixed with unit_fp_ to differentiate from any other type of
# unit test executables.
function(add_unit_test _target)
  set(_source ${_target}Test.cpp)
  set(_target "unit_fp_${_target}")
  add_executable(${_target} ${_source})

  target_link_libraries(${_target}
    ${LIB_PROCESSOR}
    DummyUDPProcessPlugin
    Hdf5Plugin
    ParameterAdjustmentPlugin
    OffsetAdjustmentPlugin
    LiveViewPlugin
    SumPlugin
    GapFillPlugin
    RawFileWriterPlugin
    ParameterPublishPlugin
    ${Boost_LIBRARIES}
    ${LOG4CXX_LIBRARIES}
    ${ZEROMQ_LIBRARIES}
    ${HDF5_LIBRARIES}
    ${HDF5_HL_LIBRARIES}
  )

  # Link for BloscPlugin if Blosc is present
  if (${BLOSC_FOUND})
    target_link_libraries(${_target} ${BLOSC_LIBRARIES} BloscPlugin)
  endif()

  if (${KAFKA_FOUND})
    target_link_libraries(${_target} ${KAFKA_LIBRARIES} KafkaProducerPlugin)
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    # librt required for timing functions
    find_library(REALTIME_LIBRARY NAMES rt)
    find_library(PTHREAD_LIBRARY NAMES pthread)
    target_link_libraries(${_target} ${REALTIME_LIBRARY} ${PTHREAD_LIBRARY})
  endif()

  install(TARGETS ${_target} RUNTIME DESTINATION bin)
endfunction()

set(CMAKE_INCLUDE_CURRENT_DIR on)
add_definitions(-DBOOST_TEST_DYN_LINK)
add_definitions(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(
  ${FRAMEPROCESSOR_DIR}/include
  ${HDF5_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${LOG4CXX_INCLUDE_DIRS}/..
  ${ZEROMQ_INCLUDE_DIRS}
)
add_definitions(${HDF5_DEFINITIONS})

add_unit_test(Acquisition)
add_unit_test(DataBlock)
add_unit_test(DummyUDPProcessPlugin)
add_unit_test(FileWriterPlugin)
add_unit_test(GapFillPlugin)
add_unit_test(HDF5File)
add_unit_test(LiveViewPlugin)
add_unit_test(MetaMessage)
add_unit_test(OffsetAdjustmentPlugin)
add_unit_test(ParameterAdjustmentPlugin)
add_unit_test(RawFileWriterPlugin)
add_unit_test(SumPlugin)

if (${BLOSC_FOUND})
  include_directories(${BLOSC_INCLUDE_DIR})
  add_unit_test(BloscPlugin)
endif()

if (${KAFKA_FOUND})
  include_directories(${KAFKA_INCLUDE_DIR})
  add_unit_test(KafkaProducerPlugin)
endif()
