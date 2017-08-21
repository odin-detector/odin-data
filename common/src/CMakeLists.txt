# Add library for odin-data shared functionality
include_directories(${Boost_INCLUDE_DIRS} ${ZEROMQ_INCLUDE_DIRS})

file(GLOB LIB_SOURCES *.cpp)

add_library(${COMMON_LIBRARY} SHARED ${LIB_SOURCES})

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    # libdl required for dlopen etc functions
    find_library(DYNAMIC_LOADER_LIBRARY
                 NAMES dl)
    target_link_libraries(${COMMON_LIBRARY} ${DYNAMIC_LOADER_LIBRARY} )
endif()

target_link_libraries(${COMMON_LIBRARY} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES})

install(TARGETS ${COMMON_LIBRARY} DESTINATION lib)
