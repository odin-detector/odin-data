include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(
  ${FRAMERECEIVER_DIR}/include
  ${Boost_INCLUDE_DIRS}
  ${LOG4CXX_INCLUDE_DIRS}/..
  ${ZEROMQ_INCLUDE_DIRS}
)

# Build list of test source files from current dir
file(GLOB TEST_SOURCES *.cpp)

# In order to obtain coverage stats for these tests the main source files need to be built
# here, rather than using the prebuilt libraries.
# This is required for the common code and the frameReceiver code
file(GLOB APP_SOURCES
  ${FRAMERECEIVER_DIR}/src/*.cpp
  ${COMMON_DIR}/src/*.cpp
)

# Exclude the application main and the registering of the libraries
file(GLOB APP_MAIN_SOURCE ${FRAMERECEIVER_DIR}/src/FrameReceiverApp.cpp)
file(GLOB LIB_SOURCES ${FRAMERECEIVER_DIR}/src/*Lib.cpp)
list(REMOVE_ITEM APP_SOURCES ${APP_MAIN_SOURCE})
list(REMOVE_ITEM APP_SOURCES ${LIB_SOURCES})

# Add test and project source files to executable
add_executable(frameReceiverTest
  ${TEST_SOURCES}
  ${APP_SOURCES}
)

# Define libraries to link against, this is only the libraries for which no coverage is required
target_link_libraries(frameReceiverTest
  ${Boost_LIBRARIES}
  ${LOG4CXX_LIBRARIES}
  ${ZEROMQ_LIBRARIES}
)

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
    # librt required for timing functions
    find_library(REALTIME_LIBRARY NAMES rt)
    find_library(PTHREAD_LIBRARY NAMES pthread)
    target_link_libraries(frameReceiverTest
      ${PTHREAD_LIBRARY}
      ${REALTIME_LIBRARY}
    )
endif()

add_code_coverage(frameReceiverTest)
install(TARGETS frameReceiverTest RUNTIME DESTINATION bin)
