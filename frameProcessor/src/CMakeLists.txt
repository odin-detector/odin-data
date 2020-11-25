
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB LIB_SOURCES DataBlock.cpp
                      DataBlockPool.cpp
                      FrameProcessorPlugin.cpp
                      FrameMetaData.cpp
                      Frame.cpp
                      SharedBufferFrame.cpp
                      DataBlockFrame.cpp
                      MetaMessage.cpp
                      MetaMessagePublisher.cpp
                      IFrameCallback.cpp
                      CallDuration.cpp
                      WatchdogTimer.cpp )

# Add library for common plugin code
add_library(${LIB_PROCESSOR} SHARED ${LIB_SOURCES})
target_link_libraries(${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS ${LIB_PROCESSOR} DESTINATION lib)

file(GLOB APP_SOURCES FrameProcessorController.cpp
                      SharedMemoryController.cpp
                      SharedMemoryParser.cpp
                      SocketHandler.cpp )

add_executable(frameProcessor ${APP_SOURCES} FrameProcessorApp.cpp)

message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})

target_link_libraries(frameProcessor ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${COMMON_LIBRARY})
if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    find_library(PTHREAD_LIBRARY
             NAMES pthread)
    target_link_libraries(frameProcessor ${PTHREAD_LIBRARY} )
endif()

install(TARGETS frameProcessor RUNTIME DESTINATION bin)

# Add library for dummy plugin
add_library(DummyUDPProcessPlugin SHARED DummyUDPProcessPlugin.cpp DummyUDPProcessPluginLib.cpp)
target_link_libraries(DummyUDPProcessPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS DummyUDPProcessPlugin DESTINATION lib)

# Add library for Blosc plugin
if (${BLOSC_FOUND})
  include_directories(${BLOSC_INCLUDE_DIR})
  add_library(BloscPlugin SHARED BloscPlugin.cpp BloscPluginLib.cpp)
  target_link_libraries(BloscPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${BLOSC_LIBRARIES} ${COMMON_LIBRARY})
  install(TARGETS BloscPlugin DESTINATION lib)
endif()

# Add library for HDF5 writer plugin
add_library(Hdf5Plugin SHARED FileWriterPlugin.cpp FileWriterPluginLib.cpp HDF5File.cpp Acquisition.cpp)
target_link_libraries(Hdf5Plugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS Hdf5Plugin DESTINATION lib)

# Add library for ParameterAdjustment plugin
add_library(ParameterAdjustmentPlugin SHARED ParameterAdjustmentPlugin.cpp ParameterAdjustmentPluginLib.cpp)
target_link_libraries(ParameterAdjustmentPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS ParameterAdjustmentPlugin DESTINATION lib)

# Add library for OffsetAdjustment plugin
add_library( OffsetAdjustmentPlugin SHARED  OffsetAdjustmentPlugin.cpp  OffsetAdjustmentPluginLib.cpp)
target_link_libraries(OffsetAdjustmentPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS OffsetAdjustmentPlugin DESTINATION lib)

# Add library for LiveView plugin
add_library(LiveViewPlugin SHARED LiveViewPlugin.cpp LiveViewPluginLib.cpp)
target_link_libraries(LiveViewPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS LiveViewPlugin DESTINATION lib)

# Add library for Sum plugin
add_library(SumPlugin SHARED SumPlugin.cpp SumPluginLib.cpp)
target_link_libraries(SumPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS SumPlugin DESTINATION lib)

# Add library for Gap Fill plugin
add_library(GapFillPlugin SHARED GapFillPlugin.cpp GapFillPluginLib.cpp)
target_link_libraries(GapFillPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS GapFillPlugin DESTINATION lib)

if (${KAFKA_FOUND})
    include_directories(${KAFKA_INCLUDE_DIR})
    # Add library for Kafka Producer plugin
    add_library(KafkaProducerPlugin SHARED KafkaProducerPlugin.cpp
                                           KafkaProducerPluginLib.cpp)
    target_link_libraries(KafkaProducerPlugin ${LIB_PROCESSOR}
        ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES}
        ${KAFKA_LIBRARIES} ${COMMON_LIBRARY})
    install(TARGETS KafkaProducerPlugin DESTINATION lib)
endif()

# Add test and project source files to executable
if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY
               NAMES rt)
  target_link_libraries( frameProcessor ${REALTIME_LIBRARY} )
endif()

