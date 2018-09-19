
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB TEST_SOURCES *.cpp)

# Add test source files to executable
add_executable(frameProcessorTest ${TEST_SOURCES})

# Define libraries to link against
target_link_libraries(frameProcessorTest
        ${LIB_PROCESSOR}
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES}
        ${HDF5HL_LIBRARIES}
        Hdf5Plugin
        UIDAdjustmentPlugin
        OffsetAdjustmentPlugin
        LiveViewPlugin
        ${COMMON_LIBRARY})

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY
               NAMES rt)
  target_link_libraries(frameProcessorTest ${REALTIME_LIBRARY} )
  find_library(PTHREAD_LIBRARY
	       NAMES pthread)
  target_link_libraries(frameProcessorTest ${PTHREAD_LIBRARY} )
endif()
