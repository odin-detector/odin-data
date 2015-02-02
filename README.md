FrameReceiver
=============

FrameReceiver is an application to enable reception of frames of detector data transmitted over a network connection as a UDP stream. FrameReceiver constructs data frames in shared memory buffers and communicates with external applications to allow processing and storage.

[![Build Status](https://travis-ci.org/percival-detector/framereceiver.svg)](https://travis-ci.org/percival-detector/framereceiver)

Building
========

FrameReceiver is built using CMake. The following libraries and packages are required:

* [CMake](http://www.cmake.org) : build management system (version >= 2.8)
* [Boost](http://www.boost.org) : portable C++ utility libraries. The following components are used - program_options, unit_test_framework, date_time, interprocess, bimap (version >= 1.41)
* [ZeroMQ](http://zeromq.org) : high-performance asynchronous messaging library (version >= 3.2.4)
* [Log4CXX](http://logging.apache.org/log4cxx/): Configurable message logger (version >= 0.10.0)

FrameReceiver is configured and built with the following commands:

    mkdir build && cd build
    cmake -DBoost_NO_BOOST_CMAKE=ON ..
    make

The Boost_NO_BOOST_CMAKE=ON flag is required on systems with Boost installed on system paths (e.g. from a package repository) where there is a bug in the installed CMake package discovery utilities. Non-default installations of the above libraries and packages can be used by setting the following flags:

* BOOST_ROOT
* ZEROMQ_ROOTDIR
* LOG4CXX_ROOT_DIR
