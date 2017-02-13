# Add library for odin-data shared functionality

file(GLOB LIB_SOURCES *.cpp)

add_library(${COMMON_LIBRARY} SHARED ${LIB_SOURCES})
target_link_libraries(${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})
