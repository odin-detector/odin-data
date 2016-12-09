
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB APP_SOURCES DataBlock.cpp 
                      DataBlockPool.cpp 
                      FileWriterController.cpp 
                      FileWriterPlugin.cpp 
                      Frame.cpp 
                      IFrameCallback.cpp 
                      SharedMemoryController.cpp 
                      SharedMemoryParser.cpp 
                      SocketHandler.cpp )

file(GLOB TEST_SOURCES *Test*.cpp)

#add_executable(filewriter ${APP_SOURCES} app.cpp)
add_executable(filewriter ${APP_SOURCES} fileWriterApp.cpp)

message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})
message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})

target_link_libraries(filewriter ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} Ipc)

# Add library for dummy plugin
add_library(DummyPlugin SHARED DummyPlugin.cpp FileWriterPlugin.cpp IFrameCallback.cpp Frame.cpp DataBlock.cpp DataBlockPool.cpp)
target_link_libraries(DummyPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} Ipc)

# Add library for HDF5 writer plugin
add_library(Hdf5Plugin SHARED FileWriter.cpp FileWriterPlugin.cpp IFrameCallback.cpp Frame.cpp DataBlock.cpp DataBlockPool.cpp)
target_link_libraries(Hdf5Plugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} Ipc)

# Add library for excalibur plugin
add_library(ExcaliburReorderPlugin SHARED ExcaliburReorderPlugin.cpp FileWriterPlugin.cpp IFrameCallback.cpp Frame.cpp DataBlock.cpp DataBlockPool.cpp)
target_link_libraries(ExcaliburReorderPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} Ipc)

# Add library for percival process plugin
add_library(PercivalProcessPlugin SHARED PercivalProcessPlugin.cpp FileWriterPlugin.cpp IFrameCallback.cpp Frame.cpp DataBlock.cpp DataBlockPool.cpp)
target_link_libraries(PercivalProcessPlugin ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES} Ipc)
            
# Add test and project source files to executable
file(GLOB TESTABLE_SOURCES DataBlock.cpp DataBlockPool.cpp IFrameCallback.cpp FileWriterPlugin.cpp Frame.cpp)
add_executable(fileWriterTest ${TEST_SOURCES} ${TESTABLE_SOURCES})

# Define libraries to link against
target_link_libraries(fileWriterTest 
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES} 
        ${HDF5HL_LIBRARIES}
        Hdf5Plugin
        Ipc) 

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY 
	  	NAMES rt)
  target_link_libraries( filewriter ${REALTIME_LIBRARY} )
  target_link_libraries( fileWriterTest ${REALTIME_LIBRARY} )
endif()
        