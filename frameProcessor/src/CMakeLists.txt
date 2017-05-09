
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB LIB_SOURCES DataBlock.cpp 
                      DataBlockPool.cpp 
                      FrameProcessorPlugin.cpp 
                      Frame.cpp 
                      IFrameCallback.cpp )

# Add library for common plugin code
set(LIB_PROCESSOR FrameProcessor) 
add_library(${LIB_PROCESSOR} SHARED ${LIB_SOURCES})
target_link_libraries(${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})

file(GLOB APP_SOURCES FrameProcessorController.cpp 
                      SharedMemoryController.cpp 
                      SharedMemoryParser.cpp 
                      SocketHandler.cpp )

file(GLOB TEST_SOURCES *Test*.cpp)

add_executable(frameProcessor ${APP_SOURCES} FrameProcessorApp.cpp)

message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})
message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})

target_link_libraries(frameProcessor ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for dummy plugin
add_library(DummyPlugin SHARED DummyPlugin.cpp)
target_link_libraries(DummyPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for HDF5 writer plugin
add_library(Hdf5Plugin SHARED FileWriterPlugin.cpp)
target_link_libraries(Hdf5Plugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for excalibur plugin
add_library(ExcaliburReorderPlugin SHARED ExcaliburReorderPlugin.cpp)
target_link_libraries(ExcaliburReorderPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})

# Add library for percival process plugin
add_library(PercivalProcessPlugin SHARED PercivalProcessPlugin.cpp)
target_link_libraries(PercivalProcessPlugin ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} ${COMMON_LIBRARY})
            
# Add test and project source files to executable
file(GLOB TESTABLE_SOURCES DataBlock.cpp DataBlockPool.cpp IFrameCallback.cpp FrameProcessorPlugin.cpp Frame.cpp)
add_executable(frameProcessorTest ${TEST_SOURCES} ${TESTABLE_SOURCES})

# Define libraries to link against
target_link_libraries(frameProcessorTest 
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES} 
        ${HDF5HL_LIBRARIES}
        Hdf5Plugin
        ${COMMON_LIBRARY}) 

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY 
	  	NAMES rt)
  target_link_libraries( frameProcessor ${REALTIME_LIBRARY} )
  target_link_libraries( frameProcessorTest ${REALTIME_LIBRARY} )
endif()
        