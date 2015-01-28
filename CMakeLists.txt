# Project name
project(frameReceiver)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${frameReceiver_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Find and add external packages required for application and test
find_package( Boost 1.41.0
	      REQUIRED
	      COMPONENTS program_options system filesystem unit_test_framework date_time)
find_package(Log4CXX 0.10.0 REQUIRED)
find_package(ZeroMQ 3.2.4 REQUIRED)

# Add include directory to include path
include_directories(include)

# Add the source subdirectory
add_subdirectory(src)

# Add the test subdirectory
add_subdirectory(test)
