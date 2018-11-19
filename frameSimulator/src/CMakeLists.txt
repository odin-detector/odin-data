
set(CMAKE_INCLUDE_CURRENT_DIR on)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic")
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMESIMULATOR_DIR}/include ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. )
add_definitions(${HDF5_DEFINITIONS})

file(GLOB LIB_SOURCES FrameSimulatorPlugin.cpp pcapFrameSimulatorPlugin.cpp)

# Add library for common plugin code
add_library(${LIB_SIMULATOR} SHARED ${LIB_SOURCES})
target_link_libraries(${LIB_SIMULATOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${ZEROMQ_LIBRARIES} ${COMMON_LIBRARY})
install(TARGETS ${LIB_SIMULATOR} DESTINATION lib)

file(GLOB APP_SOURCES
        FrameSimulatorPlugin.cpp
        FrameSimulatorOptions.cpp
        pcapFrameSimulatorPlugin.cpp)

add_executable(frameSimulator ${APP_SOURCES} FrameSimulatorApp.cpp)

target_link_libraries(frameSimulator ${LIB_PROCESSOR} ${Boost_LIBRARIES} ${LOG4CXX_LIBRARIES} ${COMMON_LIBRARY} /usr/lib64/libpcap.so )

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    find_library(PTHREAD_LIBRARY
             NAMES pthread)
    target_link_libraries(frameSimulator ${PTHREAD_LIBRARY} )
endif()

install(TARGETS frameSimulator RUNTIME DESTINATION bin)

# Add test and project source files to executable
if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY
               NAMES rt)
  target_link_libraries( frameSimulator ${REALTIME_LIBRARY} )
endif()

