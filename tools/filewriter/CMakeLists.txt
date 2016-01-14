
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})
add_definitions(${HDF5_DEFINITIONS})

file(GLOB FN_SOURCES FileWriter.cpp framenotifier_*.cpp)
file(GLOB TEST_SOURCES *Test*.cpp)

add_executable(framenotifier ${FN_SOURCES})

message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})
message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})

target_link_libraries(framenotifier ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES})

# Add test and project source files to executable
file(GLOB TESTABLE_SOURCES FileWriter.cpp framenotifier_data.cpp)
add_executable(fileWriterTest ${TEST_SOURCES} ${TESTABLE_SOURCES})

# Define libraries to link against
target_link_libraries(fileWriterTest 
        ${Boost_LIBRARIES}
        ${LOG4CXX_LIBRARIES}
        ${ZEROMQ_LIBRARIES}
        ${HDF5_LIBRARIES} ${HDF5HL_LIBRARIES}) 
