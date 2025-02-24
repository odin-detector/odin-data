# Build
## External Software Dependencies

The following libraries and packages are used:

* [CMake](http://www.cmake.org) : build management system (version >= 2.8)
* [Boost](http://www.boost.org) : portable C++ utility libraries. The following components are used - program_options, unit_test_framework, date_time, interprocess, bimap (version >= 1.41)
* [ZeroMQ](http://zeromq.org) : high-performance asynchronous messaging library (version >= 4.1.4)
* [Log4CXX](http://logging.apache.org/log4cxx/): Configurable message logger (version >= 0.10.0)
* [HDF5](https://www.hdfgroup.org/HDF5): __Optional:__ if found, the FrameProcessor application will be built (version >= 1.8.14)
* [Blosc](http://blosc.org)/[c-blosc](https://github.com/blosc/c-blosc): __Optional:__ if found, the FrameProcessor BloscPlugin will be built

## Building Dependencies

The supported development platform is Linux RedHat 7 (RHEL7) however distributions like
CentOS and Scientific Linux are essentially clones of RHEL and can be used as free
alternatives to RHEL.

Most of the dependencies are available from the RHEL (or CentOS) repositories, but a few
(log4cxx and zeromq) must be installed from the EPEL repository (or built from source).

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

These dependencies are commonly available on many linux distributions, e.g. on [arch](https://wiki.archlinux.org/):

    $ pacman -S gcc make cmake blosc boost hdf5 log4cxx

## Building odin-data

OdinData is built using CMake. The minimal commands to configure and build are:

    mkdir build && cd build
    cmake ..
    make

the name `build` has no significance and can be substituted. Some CMake flags will have
to be used to build on most systems. Non-default installations of the required libraries
and packages can be used by adding the following:

- `Boost_NO_BOOST_CMAKE=ON` and `BOOST_ROOT`
- `ZEROMQ_ROOTDIR`
- `LOG4CXX_ROOT_DIR`
- `HDF5_ROOT` - optional: the FrameProcessor will __not__ be built if HDF5 is not found.
- `BLOSC_ROOT_DIR` - optional: FrameProcessor BloscPlugin will not be built if the blosc
  library is not found

The `Boost_NO_BOOST_CMAKE=ON` flag is required on systems with Boost installed on system
paths (e.g. from a package repository) where there is a bug in the installed CMake
package discovery utilities.

Applications are compiled into the `bin` directory within `build` and the additional
python etc tools installed into tools. Sample configuration files for testing the system
are installed into `test_config`.  See the sections below further information on usage.

### Example build and install of odin-data

Create an install directory to install odin-data and plugins into:

    $ mkdir prefix

Create a build directory for CMake to use (N.B. odin-data uses CMake out-of-source build
semantics):

    $ mkdir build && cd build

Configure CMake to define use of correct packages and set up install directory:

    $ cmake -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_ROOT \
      -DZEROMQ_ROOTDIR=$ZEROMQ_ROOT -DLOG4CXX_ROOT_DIR=$LOG4CXX_ROOT \
      -DHDF5_ROOT=$HDF5_ROOT -DBLOSC_ROOT_DIR=$BLOSC_ROOT \
      -DCMAKE_INSTALL_PREFIX=<path_to_odin-data>/prefix ..

Check output from CMake for errors and correct paths (below is an example where
everything is found correctly in default locations):

    [main] Configuring folder: odin-data
    [driver] Removing /scratch/development/odin/odin-data/build/CMakeCache.txt
    [driver] Removing /scratch/development/odin/odin-data/build/CMakeFiles
    [proc] Executing command: /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_INSTALL_PREFIX:STRING=/scratch/development/odin/odin-data/prefix -DCMAKE_C_COMPILER:FILEPATH=/bin/x86_64-pc-linux-gnu-gcc-11.2.0 -S/scratch/development/odin/odin-data -B/scratch/development/odin/odin-data/build -G "Unix Makefiles"
    [cmake] Not searching for unused variables given on the command line.
    [cmake] -- The C compiler identification is GNU 11.2.0
    [cmake] -- The CXX compiler identification is GNU 11.2.0
    [cmake] -- Detecting C compiler ABI info
    [cmake] -- Detecting C compiler ABI info - done
    [cmake] -- Check for working C compiler: /bin/x86_64-pc-linux-gnu-gcc-11.2.0 - skipped
    [cmake] -- Detecting C compile features
    [cmake] -- Detecting C compile features - done
    [cmake] -- Detecting CXX compiler ABI info
    [cmake] -- Detecting CXX compiler ABI info - done
    [cmake] -- Check for working CXX compiler: /bin/c++ - skipped
    [cmake] -- Detecting CXX compile features
    [cmake] -- Detecting CXX compile features - done
    [cmake] CMake Deprecation Warning at CMakeLists.txt:5 (cmake_minimum_required):
    [cmake]   Compatibility with CMake < 2.8.12 will be removed from a future version of
    [cmake]   CMake.
    [cmake]
    [cmake]   Update the VERSION argument <min> value or use a ...<max> suffix to tell
    [cmake]   CMake that the project does not need compatibility with older versions.
    [cmake]
    [cmake]
    [cmake] -- Found Boost: /usr/lib64/cmake/Boost-1.78.0/BoostConfig.cmake (found suitable version "1.78.0", minimum required is "1.41.0") found components: program_options system filesystem unit_test_framework date_time thread regex
    [cmake]
    [cmake] Looking for log4cxx headers and libraries
    [cmake] -- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.0")
    [cmake] using pkgconfig
    [cmake] -- Checking for module 'log4cxx'
    [cmake] --   Package 'log4cxx', required by 'virtual:world', not found
    [cmake] -- Found LOG4CXX: /usr/lib/liblog4cxx.so (Required is at least version "0.10.0")
    [cmake] -- Include directories: /usr/include/log4cxx
    [cmake] -- Libraries: /usr/lib/liblog4cxx.so
    [cmake]
    [cmake] Looking for ZeroMQ headers and libraries
    [cmake] -- Checking for one of the modules 'libzmq'
    [cmake] -- Found ZEROMQ: /lib/libzmq.so (found suitable version "4.3.4", minimum required is "4.1.4")
    [cmake] -- Include directories: /usr/include
    [cmake] -- Libraries: /lib/libzmq.so
    [cmake]
    [cmake] Looking for pcap headers and libraries
    [cmake] -- Found PCAP: /usr/lib/libpcap.so (Required is at least version "1.4.0")
    [cmake] -- Performing Test PCAP_LINKS
    [cmake] -- Performing Test PCAP_LINKS - Success
    [cmake]
    [cmake] Looking for blosc headers and libraries
    [cmake] -- Found Blosc: /usr/lib/libblosc.so
    [cmake]
    [cmake] Searching for HDF5
    [cmake] -- Found HDF5: /usr/lib/libhdf5.so;/usr/lib/libsz.so;/usr/lib/libz.so;/usr/lib/libdl.a;/usr/lib/libm.so (found suitable version "1.12.1", minimum required is "1.8.14") found components: C HL
    [cmake] -- HDF5 include files:  /usr/include
    [cmake] -- HDF5 libs:           /usr/lib/libhdf5.so/usr/lib/libsz.so/usr/lib/libz.so/usr/lib/libdl.a/usr/lib/libm.so/usr/lib/libhdf5_hl.so
    [cmake] -- HDF5 defs:           -D_FORTIFY_SOURCE=2
    [cmake]
    [cmake] Determining odin-data version
    [cmake] -- Git describe version: 1.7.0-24-g170f3f7-dirty
    [cmake] -- major:1 minor:7 patch:0 sha1:g170f3f7-dirty
    [cmake] -- short version: 1.7.0
    [cmake] -- Found Doxygen: /usr/bin/doxygen (found version "1.9.3") found components: doxygen dot
    [cmake] -- Configuring done
    [cmake] -- Generating done
    [cmake] -- Build files have been written to: /scratch/development/odin/odin-data/build

This creates all the source directories and makefiles needed to compile outside the
source tree:

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

This creates five applications in bin:

    $ tree bin
    bin
    ├── frameProcessor
    ├── frameProcessorTest
    ├── frameReceiver
    ├── frameReceiverTest
    └── frameSimulator

    0 directories, 5 files

Run the unit test applications:

    $ bin/frameReceiverTest
    Running 32 test cases...

    *** No errors detected

    $ bin/frameProcessorTest
    ...

    *** No errors detected

Install odin-data into prefix:

    $ make install

Run applications (now from the root of the repository):

    $ prefix/bin/frameReceiver --help
    $ prefix/bin/frameProcessor --help
    $ prefix/bin/frameProcessor --help
