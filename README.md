FrameReceiver
=============

FrameReceiver is an application to enable reception of frames of detector data transmitted over a network connection as a UDP stream. FrameReceiver constructs data frames in shared memory buffers and communicates with external applications to allow processing and storage.

[![Build Status](https://travis-ci.org/percival-detector/framereceiver.svg)](https://travis-ci.org/percival-detector/framereceiver)
[![Stories in Ready](https://badge.waffle.io/percival-detector/framereceiver.png?label=ready&title=Ready)](https://waffle.io/percival-detector/framereceiver)

External Software Dependencies
==============================

The following libraries and packages are required:

* [CMake](http://www.cmake.org) : build management system (version >= 2.8)
* [Boost](http://www.boost.org) : portable C++ utility libraries. The following components are used - program_options, unit_test_framework, date_time, interprocess, bimap (version >= 1.41)
* [ZeroMQ](http://zeromq.org) : high-performance asynchronous messaging library (version >= 3.2.4)
* [Log4CXX](http://logging.apache.org/log4cxx/): Configurable message logger (version >= 0.10.0)

Installing dependencies
-----------------------

The supported development platform is Linux RedHat 6 (RHEL6) however distributions like CentOS and Scientific Linux are essentially clones of RHEL and can be used as free alternatives to RHEL.

Most of the dependencies are available from the RHEL (or CentOS) repositories, but a few (log4cxx and zeromq) must be installed from the EPEL repository (or built from source).

The following is an exmaple of installing dependencies on CentOS 6:

    # Install the development environment: gcc, make, etc.
    yum groupinstall "Development tools"
    
    # Install the cmake and boost libraries
    yum install cmake boost-devel
    
    # Enable the EPEL repositories - the exact method may be dependent
    # on the distribution. This works for CentOS 6:
    yum install epel-release
    
    # Install the zeromq and log4cxx libraries from the EPEL repo
    yum --enablerepo=epel install zeromq3-devel log4cxx-devel


Building
========

FrameReceiver is built using CMake and make. FrameReceiver is configured and built with the following commands:

    mkdir build && cd build
    cmake -DBoost_NO_BOOST_CMAKE=ON ..
    make

The Boost_NO_BOOST_CMAKE=ON flag is required on systems with Boost installed on system paths (e.g. from a package repository) where there is a bug in the installed CMake package discovery utilities. Non-default installations of the above libraries and packages can be used by setting the following flags:

* BOOST_ROOT
* ZEROMQ_ROOTDIR
* LOG4CXX_ROOT_DIR
