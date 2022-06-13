# Project name
project(OdinData)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIG_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_config)

# Set the common directory
set(COMMON_DIR ${OdinData_SOURCE_DIR}/common)

# Set the frameReceiver directory
set(FRAMERECEIVER_DIR ${OdinData_SOURCE_DIR}/frameReceiver)

# Set the frameProcessor directory
set(FRAMEPROCESSOR_DIR ${OdinData_SOURCE_DIR}/frameProcessor)

# Set the frameSimulator directory
set(FRAMESIMULATOR_DIR ${OdinData_SOURCE_DIR}/frameSimulator)

# set the test directory
set(TEST_DIR ${OdinData_SOURCE_DIR}/test)

# Set the name of the common library
set(COMMON_LIBRARY ${PROJECT_NAME})

# Set the name of the frameProcessor library
if (NOT APPLE)
    set(LIB_RECEIVER FrameReceiver)
    set(LIB_PROCESSOR FrameProcessor)
    set(LIB_SIMULATOR FrameSimulator)
else(NOT APPLE)
    set(LIB_RECEIVER OdinFrameReceiver)
    set(LIB_PROCESSOR OdinFrameProcessor)
    set(LIB_SIMULATOR OdinFrameSimulator)
endif(NOT APPLE)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${OdinData_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Allow macOS shared (dynamic) libraries to link with undefined symbols
if (APPLE)
	message("Allowing undefined symbols in macOS shared library linking")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif(APPLE)

# Set up RPATH handling correctly so that both built and installed targets have the appropriate
# settings - see https://cmake.org/Wiki/CMake_RPATH_handling

set(CMAKE_MACOSX_RPATH 1)

# Use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# When building, don't use the install RPATH already (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the automatically determined parts of the RPATH which point to directories outside the 
# build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# Find and add external packages required for application and test
find_package( Boost 1.41.0
	      REQUIRED
	      COMPONENTS program_options system filesystem unit_test_framework date_time thread regex)
find_package(LOG4CXX 0.10.0 REQUIRED)
find_package(ZEROMQ 4.1.4 REQUIRED)
find_package(PCAP 1.4.0 REQUIRED)
find_package(Blosc)
find_package(Kafka)

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
  find_library( HDF5_HL_LIBRARIES
                NAMES hdf5_hl
                PATHS ${HDF5_ROOT}/lib
                NO_DEFAULT_PATH )
  if (HDF5_HL_LIBRARIES)
    set(HDF5_FOUND true)
  else (HDF5_HL_LIBRARIES)
    set(HDF5_FOUND false)
  endif (HDF5_HL_LIBRARIES)
ELSE(HDF5_ROOT)
    # If no HDF5_ROOT variable has been set then we just
    # use the built-in FindHDF5.cmake...
    find_package(HDF5 1.8.14
                 COMPONENTS C HL)
ENDIF(HDF5_ROOT)
if (HDF5_FOUND)
  message(STATUS "HDF5 include files:  " ${HDF5_INCLUDE_DIRS})
  message(STATUS "HDF5 libs:           " ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
  message(STATUS "HDF5 defs:           " ${HDF5_DEFINITIONS})
ENDIF(HDF5_FOUND)

message("\nDetermining odin-data version")
include(GetGitRevisionDescription)
git_describe(GIT_DESC_STR)

string(REGEX REPLACE "^([0-9]+)[\\.-].*" "\\1" VERSION_MAJOR "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-]([0-9]+).*" "\\1" VERSION_MINOR "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-][0-9]+[\\.-]([0-9]+).*" "\\1" VERSION_PATCH "${GIT_DESC_STR}")
string(REGEX REPLACE "^[0-9]+[\\.-][0-9]+[\\.-][0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${GIT_DESC_STR}")

set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(VERSION_STR ${GIT_DESC_STR})
message("-- Git describe version: ${GIT_DESC_STR}")
message("-- major:${VERSION_MAJOR} minor:${VERSION_MINOR} patch:${VERSION_PATCH} sha1:${VERSION_SHA1}")
message("-- short version: ${VERSION_SHORT}")

configure_file(${COMMON_DIR}/include/version.h.in "${CMAKE_BINARY_DIR}/include/version.h")
include_directories(${CMAKE_BINARY_DIR}/include)

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

# Add the frameSimulator subdirectory
add_subdirectory(${FRAMESIMULATOR_DIR})

# Add the test subdirectory
add_subdirectory(${TEST_DIR})

# Add the doc subdirectory
add_subdirectory(doc)

# Add the config subdirectory (config files used for integration testing)
add_subdirectory(config)
