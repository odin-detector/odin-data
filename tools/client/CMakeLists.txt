
set(CMAKE_INCLUDE_CURRENT_DIR on)

include_directories(${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

file(GLOB FN_SOURCES framenotifier_*.cpp)

add_executable(framenotifier ${FN_SOURCES})

target_link_libraries(framenotifier ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
