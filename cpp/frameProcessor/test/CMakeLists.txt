include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR on)
add_definitions(-DBOOST_TEST_DYN_LINK)
add_definitions(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(
  ${FRAMEPROCESSOR_DIR}/include
  ${HDF5_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${LOG4CXX_INCLUDE_DIRS}/..
  ${ZEROMQ_INCLUDE_DIRS}
)
add_definitions(${HDF5_DEFINITIONS})

# Build list of test source files from current dir
file(GLOB TEST_SOURCES *.cpp)

# In order to obtain coverage stats for these tests the main source files need to be built
# here, rather than using the prebuilt libraries.
# This is required for the common code and the frameProcessor code
file(GLOB APP_SOURCES
  ${FRAMEPROCESSOR_DIR}/src/*.cpp
  ${COMMON_DIR}/src/*.cpp
)
# Exclude the application main and the registering of the libraries
file(GLOB APP_MAIN_SOURCE ${FRAMEPROCESSOR_DIR}/src/FrameProcessorApp.cpp)
file(GLOB LIB_SOURCES ${FRAMEPROCESSOR_DIR}/src/*Lib.cpp)
list(REMOVE_ITEM APP_SOURCES ${APP_MAIN_SOURCE})
list(REMOVE_ITEM APP_SOURCES ${LIB_SOURCES})

# If the Blosc or Kafka libraries are not available then their app code and test code
# must be removed from the source file lists.
if (${BLOSC_FOUND})
  include_directories(${BLOSC_INCLUDE_DIR})
else()
  list(REMOVE_ITEM APP_SOURCES ${FRAMEPROCESSOR_DIR}/src/BloscPlugin.cpp)
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/BloscPluginTest.cpp)
endif()

if (${KAFKA_FOUND})
  include_directories(${KAFKA_INCLUDE_DIR})
else()
  list(REMOVE_ITEM APP_SOURCES ${FRAMEPROCESSOR_DIR}/src/KafkaProducerPlugin.cpp)
  list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/KafkaProducerPluginTest.cpp)
endif()

# Add test source files to executable
add_executable(frameProcessorTest
  ${TEST_SOURCES}
  ${APP_SOURCES}
)

# Define libraries to link against, this is only the libraries for which no coverage is required
target_link_libraries(frameProcessorTest
  ${Boost_LIBRARIES}
  ${LOG4CXX_LIBRARIES}
  ${ZEROMQ_LIBRARIES}
  ${HDF5_LIBRARIES}
  ${HDF5_HL_LIBRARIES}
)

# Link for BloscPlugin if Blosc is present
if (${BLOSC_FOUND})
  target_link_libraries(frameProcessorTest ${BLOSC_LIBRARIES} BloscPlugin)
endif()

if (${KAFKA_FOUND})
  target_link_libraries(frameProcessorTest ${KAFKA_LIBRARIES} KafkaProducerPlugin)
endif()

if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
  # librt required for timing functions
  find_library(REALTIME_LIBRARY NAMES rt)
  find_library(PTHREAD_LIBRARY NAMES pthread)
  target_link_libraries(frameProcessorTest
    ${REALTIME_LIBRARY}
    ${PTHREAD_LIBRARY}
  )
endif()

add_code_coverage(frameProcessorTest)
install(TARGETS frameProcessorTest RUNTIME DESTINATION bin)
