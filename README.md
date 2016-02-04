FrameReceiver
=============

FrameReceiver is an application to enable reception of frames of detector data transmitted over a network connection 
as a UDP stream. FrameReceiver constructs data frames in shared memory buffers and communicates with external 
applications to allow processing and storage.

[![Build Status](https://travis-ci.org/percival-detector/framereceiver.svg)](https://travis-ci.org/percival-detector/framereceiver)
[![Stories in Ready](https://badge.waffle.io/percival-detector/framereceiver.png?label=ready&title=Ready)](https://waffle.io/percival-detector/framereceiver)
[![Stories in In Progress](https://badge.waffle.io/percival-detector/framereceiver.png?label=In%20Progress&title=In%20Progress)](https://waffle.io/percival-detector/framereceiver)

External Software Dependencies
==============================

The following libraries and packages are required:

* [CMake](http://www.cmake.org) : build management system (version >= 2.8)
* [Boost](http://www.boost.org) : portable C++ utility libraries. The following components are used - program_options, unit_test_framework, date_time, interprocess, bimap (version >= 1.41)
* [ZeroMQ](http://zeromq.org) : high-performance asynchronous messaging library (version >= 3.2.4)
* [Log4CXX](http://logging.apache.org/log4cxx/): Configurable message logger (version >= 0.10.0)
* [HDF5](https://www.hdfgroup.org/HDF5): __Optional:__ if found, the filewriter application will be built (version >= 1.8.14) 

Installing dependencies
-----------------------

The supported development platform is Linux RedHat 6 (RHEL6) however distributions like CentOS and Scientific 
Linux are essentially clones of RHEL and can be used as free alternatives to RHEL.

Most of the dependencies are available from the RHEL (or CentOS) repositories, but a few (log4cxx and zeromq) 
must be installed from the EPEL repository (or built from source).

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

The Boost_NO_BOOST_CMAKE=ON flag is required on systems with Boost installed on system paths (e.g. from a package 
repository) where there is a bug in the installed CMake package discovery utilities. Non-default installations of 
the above libraries and packages can be used by setting the following flags:

* BOOST_ROOT
* ZEROMQ_ROOTDIR
* LOG4CXX_ROOT_DIR
* HDF5_ROOT

Applications are compiled into the `bin` directory within `build` and the additional python etc tools installed into 
tools. Sample configuration files for testing the system are installed into `test_config`.  See the sections below 
further information on usage. 


Usage
=====

In addition to the core FrameReceiver application, there are a number of associated tools that can be used in 
conjunction to operate a system. These are described below.

Running the FrameReceiver
-------------------------

	bin/frameReceiver --help
	usage: frameReceiver [options]
	
	
	Generic options:
	  -h [ --help ]         Print this help message
	  -v [ --version ]      Print program version string
	  -c [ --config ] arg   Specify program configuration file
	
	Configuration options:
	  -d [ --debug ] arg (=0)                Set the debug level
	  -n [ --node ] arg (=1)                 Set the frame receiver node ID
	  -l [ --logconfig ] arg                 Set the log4cxx logging configuration 
	                                         file
	  -m [ --maxmem ] arg (=1048576)         Set the maximum amount of shared 
	                                         memory to allocate for frame buffers
	  -s [ --sensortype ] arg (=unknown)     Set the sensor type to receive frame 
	                                         data from
	  -p [ --port ] arg (=8989,8990)         Set the port to receive frame data on
	  -i [ --ipaddress ] arg (=0.0.0.0)      Set the IP address of the interface to
	                                         receive frame data on
	  --sharedbuf arg (=FrameReceiverBuffer) Set the name of the shared memory 
	                                         frame buffer
	  --frametimeout arg (=1000)             Set the incomplete frame timeout in ms
	  -f [ --frames ] arg (=0)               Set the number of frames to receive 
	                                         before terminating
	  --packetlog arg (=0)                   Enable logging of packet diagnostics 
	                                         to file
	  --rxbuffer arg (=30000000)             Set UDP receive buffer size


Running the FileWriter
----------------------

	bin/filewriter --help
	usage: filewriter [options]
	
	
	Generic options:
	  -h [ --help ]         Print this help message
	  -c [ --config ] arg   Specify program configuration file
	
	Configuration options:
	  -l [ --logconfig ] arg                 Set the log4cxx logging configuration 
	                                         file
	  --ready arg (=tcp://127.0.0.1:5001)    Ready ZMQ endpoint from frameReceiver
	  --release arg (=tcp://127.0.0.1:5002)  Release frame ZMQ endpoint from 
	                                         frameReceiver
	  -f [ --frames ] arg (=1)               Set the number of frames to be 
	                                         notified about before terminating
	  --sharedbuf arg (=FrameReceiverBuffer) Set the name of the shared memory 
	                                         frame buffer
	  -o [ --output ] arg                    Name of HDF5 file to write frames to 
	                                         (default: no file writing)
	  -p [ --processes ] arg (=1)            Number of concurrent file writer 
	                                         processes
	  -r [ --rank ] arg (=0)                 The rank (index number) of the current
	                                         file writer process in relation to the
	                                         other concurrent ones



Using the python tools
----------------------

Several python scripts in this area can be used to emulate parts of the Percival
data acquisition system. This is a summary of the tools and instructions on how to
operate them. There are two options for using the tools: 

* Create a virtual python environment, then install packages and the tools into it.
* Install required python packages and set PYTHONPATH to point at the appropriate location

The virtual environment is the preferred solution since it does not require system privileges
to install packages.


Installing a Virtual Python Environment
-------------------------------------------

Ensure you have [virtualenv](https://pypi.python.org/pypi/virtualenv) installed in your python environment 
first. Then create and activate a new virtualenv - and finally install the required dependencies with the
following commands:

    # Ensure you are in the "build" directory created above otherwise move into it
    cd <project root>/build
    
    # First create the virtual environment.
    # This need to be done only once for your working copy
    virtualenv -p /path/to/favourite/python venv
    
    # Activate the venv - this need to be done for each shell you work in
    source venv/bin/activate
    
    # Move into the python directory to install
    cd lib/python
    
    # Install the required dependencies.
    # This step is only required once for each virtualenv
    pip install -r requirements.txt
    
    # Install the tools into the environment
    # The develop argument allows the underlying tools to be updated from the source in the 
    # build step without requiring the setup script to be run-executed each time
    python setup.py develop
    

Having set up the virtual environment and installed packages and tools, it can be subsequently
reused simply by activating it with the `source venv/bin/activate` command. The virtual env can 
be exited with the command `deactivate` at any time.

Installing the required python packages manually
------------------------------------------------

This option is less preferred since system privileges are typically required to install packages
and it is necessary to ensure that PYTHONPATH is set appropriately. The following steps are
required:

	# Install required python packages using pip. This requires system
	# privileges typically, e.g. sudo
	# If pip is not installed in your python environment, packages can also be
	# downloaded and installed manually: seek local expert advice for this if
	# necessary
	pip install posix_ipc
	pip install numpy
	pip install h5py
	pip install pyzmq
	
The `h5py` python bindings require the HDF5 libraries to be installed on your system (see above). If
not installed into a standard location, set the environment variable `HDF5_DIR` to point to the installation.
See the [h5py install instructions](http://docs.h5py.org/en/latest/build.html) for more information.

The `pyzmq` python bindings for ZeroMQ do not require ZeroMQ to be installed, although this is requried to
build and run the compiled applications. There is, however, a potential conflict between `pyzmq` and ZeroMQ v3 
libraries on RedHat-based systems, causing python tools to fail when importing the bindings. If this occurs,
delete the bindings with the command `pip unintall pyzmq` and reinstall with 
`pip install pyzmq --install-option='--zmq=bundled'` to force installation of a local ZeroMQ library.

Available python tools
----------------------

The following describe the available tools, and their configurations.

*frame_producer*

*frame_processor*

*emulator_client*

*port_counters*

