
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB APP_SOURCES DataBlock.cpp 
                      DataBlockPool.cpp 
                      FileWriter.cpp 
                      FileWriterController.cpp 
                      FileWriterPlugin.cpp 
                      Frame.cpp 
                      IFrameCallback.cpp 
                      IJSONCallback.cpp 
                      JSONMessage.cpp 
                      JSONPublisher.cpp 
                      JSONSubscriber.cpp 
                      SharedMemoryController.cpp 
                      SharedMemoryParser.cpp 
                      SocketHandler.cpp )

file(GLOB TEST_SOURCES *Test*.cpp)

#add_executable(filewriter ${APP_SOURCES} app.cpp)
add_executable(filewriter ${APP_SOURCES} fileWriterApp.cpp)

message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})
message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})

target_link_libraries(filewriter ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})

# Add library for dummy plugin
add_library(DummyPlugin SHARED DummyPlugin.cpp)
# Add library for excalibur plugin
add_library(ExcaliburReorderPlugin SHARED ExcaliburReorderPlugin.cpp)
# Add library for percival process plugin
add_library(PercivalProcessPlugin SHARED PercivalProcessPlugin.cpp)
            
# Add test and project source files to executable
file(GLOB TESTABLE_SOURCES JSONMessage.cpp DataBlock.cpp DataBlockPool.cpp IFrameCallback.cpp FileWriterPlugin.cpp FileWriter.cpp Frame.cpp)
add_executable(fileWriterTest ${TEST_SOURCES} ${TESTABLE_SOURCES})

# Define libraries to link against
target_link_libraries(fileWriterTest 
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES}) 

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY 
	  	NAMES rt)
  target_link_libraries( filewriter ${REALTIME_LIBRARY} )
  target_link_libraries( fileWriterTest ${REALTIME_LIBRARY} )
endif()
        