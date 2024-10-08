set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMEPROCESSOR_DIR}/include ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB TEST_SOURCES
    FrameProcessorUnitTestMain.cpp
    FrameProcessorTest.cpp
    GapFillPluginTest.cpp
    MetaMessageTest.cpp
    DummyUDPProcessPluginTest.cpp
)
# Add tests for BloscPlugin if Blosc is present
if (${BLOSC_FOUND})
  list(APPEND TEST_SOURCES BloscPluginTest.cpp)
  include_directories(${BLOSC_INCLUDE_DIR})
endif()

if (${KAFKA_FOUND})
    list(APPEND TEST_SOURCES KafkaProducerPluginTest.cpp)
    include_directories(${KAFKA_INCLUDE_DIR})
endif()

# Add test source files to executable
add_executable(frameProcessorTest ${TEST_SOURCES})

# Define libraries to link against
target_link_libraries(frameProcessorTest
        ${LIB_PROCESSOR}
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        Hdf5Plugin
        ParameterAdjustmentPlugin
        OffsetAdjustmentPlugin
        LiveViewPlugin
        SumPlugin
        GapFillPlugin
        DummyUDPProcessPlugin
        ${COMMON_LIBRARY})

# Link for BloscPlugin if Blosc is present
if (${BLOSC_FOUND})
  target_link_libraries(frameProcessorTest ${BLOSC_LIBRARIES} BloscPlugin)
endif()

if (${KAFKA_FOUND})
  target_link_libraries(frameProcessorTest ${KAFKA_LIBRARIES}
      KafkaProducerPlugin)
endif()

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY
               NAMES rt)
  target_link_libraries(frameProcessorTest ${REALTIME_LIBRARY} )
  find_library(PTHREAD_LIBRARY
	       NAMES pthread)
  target_link_libraries(frameProcessorTest ${PTHREAD_LIBRARY} )
endif()

install(TARGETS frameProcessorTest RUNTIME DESTINATION bin)