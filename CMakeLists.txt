# Project name
project(odinData)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIG_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_config)

# Set the common directory
set(COMMON_DIR ${odinData_SOURCE_DIR}/common)

# Set the frameReceiver directory
set(FRAMERECEIVER_DIR ${odinData_SOURCE_DIR}/frameReceiver)

# Set the frameProcessor directory
set(FRAMEPROCESSOR_DIR ${odinData_SOURCE_DIR}/frameProcessor)

# Set the name of the common library
set(COMMON_LIBRARY ${PROJECT_NAME})

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${odinData_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Allow macOS shared (dynamic) libraries to link with undefined symbols
if (APPLE)
	message("Allowing undefined symbols in macOS shared library linking")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif(APPLE)

# Find and add external packages required for application and test
find_package( Boost 1.41.0
	      REQUIRED
	      COMPONENTS program_options system filesystem unit_test_framework date_time thread)
find_package(Log4CXX 0.10.0 REQUIRED)
find_package(ZeroMQ 3.2.4 REQUIRED)

# find package HDF5
# FindHDF5.cmake is essentially broken and does not allow
# to properly override the search path by setting HDF5_ROOT.
# This is a work-around to allow setting a custom HDF5
# installation location to search.
message("\nSearching for HDF5")

IF (HDF5_ROOT)
  message(STATUS "HDF5_ROOT set: " ${HDF5_ROOT})
  set( ENV{HDF5_ROOT} ${HDF5_ROOT} )
  find_path( HDF5_INCLUDE_DIRS
             NAMES hdf5.h hdf5_hl.h
             PATHS ${HDF5_ROOT}/include
             NO_DEFAULT_PATH )
  find_library( HDF5_LIBRARIES
                NAMES hdf5
                PATHS ${HDF5_ROOT}/lib
                NO_DEFAULT_PATH )
  find_library( HDF5HL_LIBRARIES
                NAMES hdf5_hl
                PATHS ${HDF5_ROOT}/lib
                NO_DEFAULT_PATH )
  if (HDF5HL_LIBRARIES)
    set(HDF5_FOUND true)
  else (HDF5HL_LIBRARIES)
    set(HDF5_FOUND false)
  endif (HDF5HL_LIBRARIES)
ELSE(HDF5_ROOT)
    # If no HDF5_ROOT variable has been set then we just
    # use the built-in FindHDF5.cmake...
    find_package(HDF5 1.8.14
                 COMPONENTS C HL)
ENDIF(HDF5_ROOT)

# Add common/include directory to include path
include_directories(${COMMON_DIR}/include)

# Add the common subdirectory
add_subdirectory(${COMMON_DIR})

# Add the frameReceiver subdirectory
add_subdirectory(${FRAMERECEIVER_DIR})

IF (NOT HDF5_FOUND)
    message(WARNING "Did not find HDF5. Not building frameProcessor application")
ELSE (NOT HDF5_FOUND)
    add_subdirectory(${FRAMEPROCESSOR_DIR})
ENDIF (NOT HDF5_FOUND)

# Add the tools subdirectory
add_subdirectory(tools)

# Add the doc subdirectory
add_subdirectory(doc)

# Add the config subdirectory (config files used for integration testing)
add_subdirectory(config)
