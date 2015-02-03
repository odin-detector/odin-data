
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

add_executable(frameReceiverTest IpcMessageTest.cpp)

find_library(REALTIME_LIBRARY 
		NAMES rt)
		
target_link_libraries(frameReceiverTest 
		${Boost_LIBRARIES} 
		${LOG4CXX_LIBRARIES} 
		${ZEROMQ_LIBRARIES}
		${REALTIME_LIBRARY})
		
ADD_TEST(frameReceiverTest frameReceiverTest)
