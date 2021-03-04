#
# - FindZEROMQ module
# Module to find ZEROMQ package on Windows, MacOSX and Linux.
# On windows are are looking for the libraries in the default locations when the
# user follows the build instructions on the package website.   On Linux we are
# using pkg_config to provide a starting point to look for the package.  If
# the default method doesn't succeed, one can either add the  location of
# ZEROMQ in the CMAKE_PREFIX_PATH variable, or set the  ZEROMQ_ROOTDIR.
#
# Usage of this module as follows:
#   find_package(ZEROMQ)
#
# After running the find, the variables below will be defined:
#   ZEROMQ_FOUND              System has ZEROMQ libs/headers
#   ZEROMQ_INCLUDE_DIRS       The location of ZEROMQ headers
#   ZEROMQ_LIBRARIES          The ZEROMQ libraries
#   ZEROMQ_VERSION            The location of ZEROMQ headers
#

#=============================================================================
# Copyright 2013 Atif Mahmood <atif1996@gmail.com>
# License: GNU LGPL v 3.0, see http://www.gnu.org/licenses/lgpl-3.0-standalone.html
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

message("\nLooking for ZeroMQ headers and libraries")

if (ZEROMQ_ROOTDIR)
	message(STATUS "Root dir: ${ZEROMQ_ROOTDIR}")
endif()

if (UNIX)
  find_package(PkgConfig)
  pkg_search_module( zeromq_pkg libzmq)
endif()

find_path(ZEROMQ_INCLUDE_DIRS
  zmq.h
  HINTS
    ${ZEROMQ_ROOTDIR}
    ${zeromq_pkg_INCLUDEDIR}
  PATH_SUFFIXES
    include
  DOC
    "Include Directory for ZEROMQ"
  )

function(extract_version_value value_name file_name value)
  file(STRINGS ${file_name} val REGEX "${value_name} .")
  string(FIND ${val} " " last REVERSE)
  string(SUBSTRING ${val} ${last} -1 val)
  string(STRIP ${val} val)
  set(${value} ${val} PARENT_SCOPE)
endfunction(extract_version_value)

extract_version_value("ZMQ_VERSION_MAJOR" ${ZEROMQ_INCLUDE_DIRS}/zmq.h MAJOR)
extract_version_value("ZMQ_VERSION_MINOR" ${ZEROMQ_INCLUDE_DIRS}/zmq.h MINOR)
extract_version_value("ZMQ_VERSION_PATCH" ${ZEROMQ_INCLUDE_DIRS}/zmq.h PATCH)

set(ZEROMQ_VERSION "${MAJOR}.${MINOR}.${PATCH}")

set(ZEROMQ_ROOTDIR_LIB ${ZEROMQ_ROOTDIR}/lib)

find_library(ZEROMQ_LIB_REL
  NAMES
    zmq libzmq                # Standard and windows lib names
  PATH_SUFFIXES
    ${LIB_PATH_SUFFIX}
  HINTS
    ${ZEROMQ_ROOTDIR}
    ${ZEROMQ_ROOTDIR_LIB}
    ${zeromq_pkg_LIBDIR}
  )

if (ZEROMQ_LIB_DEB)
  set(ZEROMQ_LIBRARIES debug ${ZEROMQ_LIB_DEB}
                       optimized ${ZEROMQ_LIB_REL})
else()
  set(ZEROMQ_LIBRARIES ${ZEROMQ_LIB_REL})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZEROMQ
    REQUIRED_VARS ZEROMQ_LIBRARIES ZEROMQ_INCLUDE_DIRS
    VERSION_VAR ZEROMQ_VERSION
)

mark_as_advanced( ZEROMQ_LIBRARIES ZEROMQ_INCLUDE_DIRS ZEROMQ_VERSION )

if (ZEROMQ_FOUND)
  message(STATUS "Include directories: ${ZEROMQ_INCLUDE_DIRS}")
  message(STATUS "Libraries: ${ZEROMQ_LIBRARIES}")
endif ()
