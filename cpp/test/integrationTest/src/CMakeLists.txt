
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK -DBOOST_BIND_GLOBAL_PLACEHOLDERS)

include_directories(${TEST_DIR}/integrationTest/include
		${TEST_INCLUDE_DIRS}
		${HDF5_INCLUDE_DIRS}
		${Boost_INCLUDE_DIRS}
		${ZEROMQ_INCLUDE_DIRS}
		${LOG4CXX_INCLUDE_DIRS}/..)

add_definitions(${HDF5_DEFINITIONS})

file(GLOB ODINDATA_SOURCES FrameTestApp.cpp
				PropertyTreeUtility.cpp
				ControlUtility.cpp)

file(GLOB FRAMETEST_SOURCES FrameSimulatorUnitTestMain.cpp
						HDF5FrameTest.cpp
						PropertyTreeUtility.cpp)

# Add executable to run odin-data system
add_executable(odinDataTest ${ODINDATA_SOURCES})

# Add executable to run output tests
add_executable(frameTests ${FRAMETEST_SOURCES})

target_link_libraries(odinDataTest ${Boost_LIBRARIES} ${HDF5_LIBRARIES} ${LOG4CXX_LIBRARIES} ${COMMON_LIBRARY})
target_link_libraries(frameTests ${Boost_LIBRARIES} ${HDF5_LIBRARIES} ${LOG4CXX_LIBRARIES} ${COMMON_LIBRARY})

install(TARGETS odinDataTest RUNTIME DESTINATION bin)
install(TARGETS frameTests RUNTIME DESTINATION bin)
