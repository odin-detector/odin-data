# Project name
project(frameReceiver)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIG_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_config)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${frameReceiver_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Coverage configuration
option(COVERALLS "Generate coveralls data" OFF)

if (COVERALLS)
  include(Coveralls)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")

  set(FR_SRC src)
  set(FW_SRC tools/filewriter)

  set(COVERAGE_SRCS
          ${FR_SRC}/appMain.cpp ${FR_SRC}/FrameReceiverApp.cpp ${FR_SRC}/FrameReceiverRxThread.cpp
          ${FR_SRC}/FrameReceiverApp.cpp ${FR_SRC}/IpcChannel.cpp ${FR_SRC}/IpcMessage.cpp ${FR_SRC}/IpcReactor.cpp
          ${FR_SRC}/PercivalEmulatorFrameDecoder.cpp ${FR_SRC}/SharedBufferManager.cpp
          ${FW_SRC}/DataBlock.cpp ${FW_SRC}/DataBlockPool.cpp ${FW_SRC}/DataBlock.cpp ${FW_SRC}/DummyPlugin.cpp
          ${FW_SRC}/ExcaliburReorderPlugin.cpp  ${FW_SRC}/FileWriter.cpp  ${FW_SRC}/fileWriterApp.cpp
          ${FW_SRC}/FileWriterController.cpp ${FW_SRC}/FileWriterPlugin.cpp ${FW_SRC}/Frame.cpp
          ${FW_SRC}/IFrameCallback.cpp ${FW_SRC}/PercivalProcessPlugin.cpp ${FW_SRC}/SharedMemoryController.cpp
          ${FW_SRC}/SharedMemoryParser.cpp
          )

  # Create the coveralls target.
  coveralls_setup(
          "${COVERAGE_SRCS}" # The source files
          ON                 # If we should upload.
          "${PROJECT_SOURCE_DIR}/cmake")

  enable_testing()
  add_test(frame_receiver_test ${PROJECT_SOURCE_DIR}/build/bin/frameReceiverTest)
  add_test(file_writer_test ${PROJECT_SOURCE_DIR}/build/bin/fileWriterTest)

endif()

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

# Add include directory to include path
include_directories(include)

# Add the source subdirectory
add_subdirectory(src)

# Add the tools subdirectory
add_subdirectory(tools)

# Add the test subdirectory
add_subdirectory(test)

# Add the doc subdirectory
add_subdirectory(doc)

# Add the config subdirectory (config files used for integration testing)
add_subdirectory(config)

