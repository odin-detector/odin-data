Installing Dependencies
-----------------------

The supported development platform is Linux RedHat 7 (RHEL7) however distributions like CentOS and Scientific
Linux are essentially clones of RHEL and can be used as free alternatives to RHEL.

Most of the dependencies are available from the RHEL (or CentOS) repositories, but a few (log4cxx and zeromq)
must be installed from the EPEL repository (or built from source).

The following is an example of installing dependencies on CentOS 7:

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
--------

OdinData is built using CMake and make. The minimal commands to configure and build are:

    mkdir build && cd build
    cmake ..
    make

the name `build` has no significance and can be substituted. Some cmake flags will have to be used to build on most systems.
Non-default installations of the required libraries and packages can be used by setting the following flags:

* Boost_NO_BOOST_CMAKE=ON and BOOST_ROOT
* ZEROMQ_ROOTDIR
* LOG4CXX_ROOT_DIR
* HDF5_ROOT - optional: the frameProcessor will __not__ be built if HDF5 is not found.
* BLOSC_ROOT_DIR - optional: BloscPlugin will not be built if the blosc library is not found

The Boost_NO_BOOST_CMAKE=ON flag is required on systems with Boost installed on system paths (e.g. from a package
repository) where there is a bug in the installed CMake package discovery utilities.

Applications are compiled into the `bin` directory within `build` and the additional python etc tools installed into
tools. Sample configuration files for testing the system are installed into `test_config`.  See the sections below
further information on usage.

### Example Build and Install of OdinData

Create an install directory to install odin-data and plugins into:

    mkdir install

Create a build directory for CMake to use N.B. ODIN uses CMake out-of-source build semantics:

    $ cd odin-data
    $ mkdir build && cd build

Configure CMake to define use of correct packages and set up install directory:

    $ cmake -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_ROOT \
      -DZEROMQ_ROOTDIR=$ZEROMQ_ROOT -DLOG4CXX_ROOT_DIR=$LOG4CXX_ROOT \
      -DHDF5_ROOT=$HDF5_ROOT -DBLOSC_ROOT_DIR=$BLOSC_ROOT \
      -DCMAKE_INSTALL_PREFIX=~/develop/projects/<project-name>/install ..

(This is verbose and error-prone but you only have to do it once per setup).

Check output from CMake for errors and correct paths:

    -- The C compiler identification is GNU 4.4.7
    -- The CXX compiler identification is GNU 4.4.7
    -- Check for working C compiler: /usr/bin/cc
    -- Check for working C compiler: /usr/bin/cc -- works
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Check for working CXX compiler: /usr/bin/c++
    -- Check for working CXX compiler: /usr/bin/c++ -- works
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Boost version: 1.48.0
    -- Found the following Boost libraries:
    --   program_options
    --   system
    --   filesystem
    --   unit_test_framework
    --   date_time
    --   thread

    Looking for log4cxx headers and libraries
    -- Root dir: /aeg_sw/tools/CentOS6-x86_64/log4cxx/0-10-0/prefix
    -- Found PkgConfig: /usr/bin/pkg-config (found version "0.23")
    -- Found LOG4CXX: /usr/lib64/liblog4cxx.so
    -- Include directories: /usr/include/log4cxx
    -- Libraries: /usr/lib64/liblog4cxx.so

    Looking for ZeroMQ headers and libraries
    -- Root dir: /aeg_sw/tools/CentOS6-x86_64/zeromq/4-2-1/prefix
    -- checking for one of the modules 'libzmq'
    -- Found ZEROMQ: /aeg_sw/tools/CentOS6-x86_64/zeromq/4-2-1/prefix/lib/libzmq.so
    -- Include directories: /aeg_sw/tools/CentOS6-x86_64/zeromq/4-2-1/prefix/include
    -- Libraries: /aeg_sw/tools/CentOS6-x86_64/zeromq/4-2-1/prefix/lib/libzmq.so

    Looking for pcap headers and libraries
    -- Found PCAP: /usr/lib64/libpcap.so (Required is at least version "1.4.0")
    -- Performing Test PCAP_LINKS
    -- Performing Test PCAP_LINKS - Success

    Looking for blosc headers and libraries
    -- Searching Blosc Root dir: /aeg_sw/tools/CentOS7-x86_64/c-blosc/1-13-5/prefix
    -- Found Blosc: /aeg_sw/tools/CentOS7-x86_64/c-blosc/1-13-5/prefix/lib/libblosc.so

    Searching for HDF5
    -- HDF5_ROOT set: /aeg_sw/tools/CentOS7-x86_64/hdf5/1-10-4/prefix
    Determining odin-data version
    -- Git describe version: 0.7.0
    -- major:0 minor:7 patch:0 sha1:0.7.0
    -- short version: 0.7.0
    -- HDF5 include files:  /aeg_sw/tools/CentOS7-x86_64/hdf5/1-10-4/prefix/include
    -- HDF5 libs:           /aeg_sw/tools/CentOS7-x86_64/hdf5/1-10-4/prefix/lib/libhdf5.so/aeg_sw/tools/CentOS7-x86_64/hdf5/1-10-4/prefix/lib/libhdf5_hl.so
    -- HDF5 defs:
    -- Found Doxygen: /usr/bin/doxygen (found version "1.8.5")
    -- Configuring done
    -- Generating done
    -- Build files have been written to: <home_dir>/develop/projects/odin-demo/odin-data/build

This creates all the directories, makefiles etc needed to compile:

    $ tree -d -L 1
    .
    ├── bin
    ├── CMakeFiles
    ├── common
    ├── config
    ├── doc
    ├── frameProcessor
    ├── frameReceiver
    ├── lib
    └── tools

Compile odin-data:

    $ make -j4

Produces a lot of output first time through:

    Scanning dependencies of target CopyPythonToolModules
    Scanning dependencies of target CopyTestConfigs
    Scanning dependencies of target CopyClientMsgFiles
    Scanning dependencies of target OdinData
    [  3%] [  3%] Generating ../test_config/fp_log4cxx.xml
    Generating ../../test_config/client_msgs/reconfig_endpoints.json
    [  4%] [  6%] Generating ../test_config/fr_log4cxx.xml
    Generating ../../test_config/client_msgs/reconfig_buffer_manager.json
    [  8%] [  9%] Generating ../test_config/fr_test.config
    Generating ../../test_config/client_msgs/config_ctrl_chan_port_5010.json
    [ 11%] [ 13%] Generating ../test_config/fp_test.config
    Generating ../../test_config/client_msgs/config_ctrl_chan_port_5000.json
    [ 14%] [ 14%] Generating ../test_config/fr_test_osx.config
    Built target CopyPythonToolModules
    [ 16%] Generating ../../test_config/client_msgs/reconfig_rx_thread.json
    [ 18%] Generating ../../test_config/client_msgs/reconfig_decoder.json
    [ 19%] Generating ../test_config/fp_py_test.config
    [ 21%] Generating ../test_config/fp_py_test_osx.config
    [ 21%] Built target CopyClientMsgFiles
    [ 22%] [ 24%] [ 26%] Generating ../test_config/fp_py_test_excalibur.config
    Generating ../test_config/fr_excalibur1.config
    Generating ../test_config/fr_excalibur2.config
    [ 27%] [ 29%] Generating ../test_config/fp_excalibur1.config
    Generating ../test_config/fp_excalibur2.config
    [ 29%] Built target CopyTestConfigs
    [ 31%] Building CXX object common/src/CMakeFiles/OdinData.dir/logging.cpp.o
    [ 32%] [ 34%] Building CXX object common/src/CMakeFiles/OdinData.dir/SharedBufferManager.cpp.o
    [ 36%] Building CXX object common/src/CMakeFiles/OdinData.dir/IpcReactor.cpp.o
    Building CXX object common/src/CMakeFiles/OdinData.dir/IpcMessage.cpp.o
    [ 37%] Building CXX object common/src/CMakeFiles/OdinData.dir/IpcChannel.cpp.o
    Linking CXX shared library ../../lib/libOdinData.so
    [ 37%] Built target OdinData

    << snip >>

    Linking CXX executable ../../bin/frameReceiver
    Linking CXX executable ../../bin/frameProcessor
    [ 88%] Built target frameReceiver
    [ 90%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/FrameReceiverController.cpp.o
    [ 91%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/DummyUDPFrameDecoderLib.cpp.o
    [ 91%] Built target frameProcessor
    [ 93%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/FrameReceiverZMQRxThread.cpp.o
    Linking CXX shared library ../../lib/libHdf5Plugin.so
    [ 93%] Built target Hdf5Plugin
    [ 95%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/DummyUDPFrameDecoder.cpp.o
    [ 96%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/FrameReceiverRxThread.cpp.o
    [ 98%] Building CXX object frameReceiver/test/CMakeFiles/frameReceiverTest.dir/__/src/FrameDecoder.cpp.o
    Scanning dependencies of target frameProcessorTest
    [100%] Building CXX object frameProcessor/test/CMakeFiles/frameProcessorTest.dir/FrameProcessorTest.cpp.o
    Linking CXX executable ../../bin/frameReceiverTest
    [100%] Built target frameReceiverTest
    Linking CXX executable ../../bin/frameProcessorTest
    [100%] Built target frameProcessorTest

This compiles five applications into build/bin:

    $ tree bin
    bin
    ├── frameProcessor
    ├── frameProcessorTest
    ├── frameReceiver
    ├── frameReceiverTest
    └── frameSimulator

    0 directories, 5 files

        Run the unit test applications (optional):

    $ bin/frameReceiverTest
    Running 32 test cases...

    *** No errors detected
    $ bin/frameProcessorTest
    bin/frameProcessorTest
    Running 16 test cases...
    TRACE - Frame constructed
    TRACE - copy_data called with size: 24 bytes

    << snip >>

    *** No errors detected

Install odin-data into <project-name>/install directory:

    $ make install
    $ make install
    [  8%] Built target OdinData
    [  9%] Built target FrameReceiver
    [ 13%] Built target DummyUDPFrameDecoder
    [ 21%] Built target frameReceiver
    [ 44%] Built target frameReceiverTest
    [ 55%] Built target FrameProcessor
    [ 57%] Built target DummyPlugin
    [ 63%] Built target Hdf5Plugin
    [ 68%] Built target frameProcessor
    [ 70%] Built target frameProcessorTest
    [ 70%] Built target CopyPythonToolModules
    [ 90%] Built target CopyTestConfigs
    [100%] Built target CopyClientMsgFiles
    Install the project...
    -- Install configuration: ""
    -- Installing: <home_dir>/develop/projects/odin-demo/install/include/ClassLoader.h

    << snip >>

    "<home_dir>/develop/projects/odin-demo/install/lib:/aeg_sw/tools/CentOS6-x86_64/boost/1-48-0/prefix/lib:/aeg_sw/tools/CentOS6-x86_64/zeromq/4-2-1/prefix/lib:/aeg_sw/tools/CentOS6-x86_64/hdf5/1-10-0/prefix/lib"

