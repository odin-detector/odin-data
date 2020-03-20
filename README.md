FrameReceiver
=============

FrameReceiver is an application to enable reception of frames of detector data transmitted over a network connection
as a UDP stream. FrameReceiver constructs data frames in shared memory buffers and communicates with external
applications to allow processing and storage.

[![Build Status](https://travis-ci.org/odin-detector/odin-data.svg)](https://travis-ci.org/odin-detector/odin-data)
[![Stories in Ready](https://badge.waffle.io/odin-detector/odin-data.png?label=ready&title=Ready)](https://waffle.io/odin-detector/odin-data)
[![Stories in In Progress](https://badge.waffle.io/odin-detector/odin-data.png?label=In%20Progress&title=In%20Progress)](https://waffle.io/odin-detector/odin-data)
[![Code Climate](https://codeclimate.com/github/odin-detector/odin-data/badges/gpa.svg)](https://codeclimate.com/github/odin-detector/odin-data)
[![Test Coverage](https://codeclimate.com/github/odin-detector/odin-data/badges/coverage.svg)](https://codeclimate.com/github/odin-detector/odin-data/coverage)

External Software Dependencies
==============================

The following libraries and packages are required:

* [CMake](http://www.cmake.org) : build management system (version >= 2.8)
* [Boost](http://www.boost.org) : portable C++ utility libraries. The following components are used - program_options, unit_test_framework, date_time, interprocess, bimap (version >= 1.41)
* [ZeroMQ](http://zeromq.org) : high-performance asynchronous messaging library (version >= 3.2.4)
* [Log4CXX](http://logging.apache.org/log4cxx/): Configurable message logger (version >= 0.10.0)
* [HDF5](https://www.hdfgroup.org/HDF5): __Optional:__ if found, the frame processor application will be built (version >= 1.8.14)
* [Blosc](http://blosc.org)/[c-blosc](https://github.com/blosc/c-blosc): __Optional:__ if found, the frame processor plugin "BloscPlugin" will be built

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
* HDF5_ROOT - optional: the frameProcessor will __not__ be built if HDF5 is not found.
* BLOSC_ROOT_DIR - optional: BloscPlugin will not be built if the blosc library is not found

Applications are compiled into the `bin` directory within `build` and the additional python etc tools installed into
tools. Sample configuration files for testing the system are installed into `test_config`.  See the sections below
further information on usage.


Usage
=====

In addition to the core FrameReceiver application, there are a number of associated tools that can be used in
conjunction to operate a system. These are described below.

Running the FrameReceiver
-------------------------

The frameReceiver is a C++ compiled application and is configured by a combination of command-line options
and equivalent parameters stored in an INI-formatted configuration file. The command-line options take
precedence over the configuration file if specified. The options and their default values can be viewed
by invoking the`--help` option:

	$ bin/frameReceiver --help
    usage: frameReceiver [options]


    Generic options:
      -h [ --help ]         Print this help message
      -v [ --version ]      Print program version string
      -c [ --config ] arg   Specify program configuration file

    Configuration options:
      -d [ --debug-level ] arg (=0)         Set the debug level
      -n [ --node ] arg (=1)                Set the frame receiver node ID
      -l [ --logconfig ] arg                Set the log4cxx logging configuration
                                            file
      -m [ --maxmem ] arg (=1048576)        Set the maximum amount of shared memory
                                            to allocate for frame buffers
      -t [ --decodertype ] arg (=unknown)   Set the decoder type to to handle data
                                            reception
      --path arg (=/home/gnx91527/work/tristan/odin-data/build_name/lib/)
                                            Path to load the decoder library from
      --rxtype arg (=udp)                   Set the interface to use for receiving
                                            frame data (udp or zmq)
      -p [ --port ] arg (=8989,8990)        Set the port to receive frame data on
      -i [ --ipaddress ] arg (=0.0.0.0)     Set the IP address of the interface to
                                            receive frame data on
      --sharedbuf arg (=FrameReceiverBuffer)
                                            Set the name of the shared memory frame
                                            buffer
      --frametimeout arg (=1000)            Set the incomplete frame timeout in ms
      -f [ --frames ] arg (=0)              Set the number of frames to receive
                                            before terminating
      --packetlog arg (=0)                  Enable logging of packet diagnostics to
                                            file
      --rxbuffer arg (=30000000)            Set UDP receive buffer size
      --iothreads arg (=1)                  Set number of IPC channel IO threads
      --ctrl arg (=tcp://*:5000)            Set the control channel endpoint
      --ready arg (=tcp://*:5001)           Set the frame ready channel endpoint
      --release arg (=tcp://*:5002)         Set the frame release channel endpoint
      -j [ --json_file ] arg                Path to a JSON configuration file to
                                            submit to the application


The meaning of the configuration options are as follows:

* `-h` or `--help`

   Print the help message shown above.

* `-v` or `--version`

   Print the program version string (to be implemented).

* `-c` or `--config`

   Specify the program configuration file to be loaded.

* `-d` or `--debug`

   Specify the debug level. Increasing the value increases the verbosity of the debug output.

* `-n` or `--node`

   Set the frame receiver node ID. Identifies the node in a multi-receiver system.

* `-l` or `--logconfig`

   Set the log4cxx logging configuration file to use, which configures the format and destination
   of logging output from the application. See the README.md file in the `config` directory for
   more information.

* `-m` or `--maxmem`

   Set the maximum amount of shared memory to allocate for frame buffers. This memory is where
   received frames are stored and handed off for processing by e.g. the fileWriter.

* `-s` or `--sensortype`

   Set the sensor type to receive frame data from. This a string parameter describing which
   type of data the receiver should expect. Currently only a type of `percivalemulator` is
   supported.

* `-p` or `--port`

   Set the port(s) to receive frame data on, specified as a comma-separated list, e.g. `8989,8990`.

* `-i`  or `--ipaddress`

   Set the the IP address to listen for data on. The default value of `0.0.0.0` listens on all
   available network interfaces.

* `--sharedbuf`

   Set the name of the shared memory frame buffer to use. Needs to match the name used by the
   downstream processing task, e.g. the fileWriter.

* `--frametimeout`

   Set the timeout in milliseconds for releasing incomplete frames (i.e. those missing
   packets) to the downtream processing task.

* `-f` or `--frames`

   Set the number of frames to receive before terminating. The frameReceiver will wait for
   those frames to be released by the processing task before terminating. The default
   value of 0 means run indefinitely.

* `--packetlog`

   Set to a non-zero value to enable logging of packet diagnostics to a separate log file,
   whose format and destination are configured in the logging configuration file. __NOTE__ Turning
   this option on will produce large quantities of output and significantly impact on the
   performance of the frameReceiver.

* `--rxbuffer`

   Set UDP receive buffer size in bytes.

An example configuration file `fr_test.config` i.s available in the `config` directory. Typical
invocation of the frameReceiver in a test would be as follows:

	bin/frameReceiver --config test_config/fr_test.config --logconfig test_config/fr_log4cxx.xml --debug 2 --frames 3



Running the FrameProcessor
--------------------------

The FrameProcessor application communicate with the FrameReceiver and receives notifications when
incoming image frames are ready to be processed and written to disk. The frame data is transferred
through a shared memory interface (i.e. the `--sharedbuf` argument) to minimise the required
number of copies. Once a frame has been processed, the memory is handed back to the FrameReceiver
to be re-used.

The application can be used as a diagnostic without actually writing data to files: if the
`-o/--output` filename is not given, the application will still communicate with the frameReceiver;
receiving and releasing frames, but without writing data to disk.

This application is intended to run as part of a large system with multiple servers concurrently
acquiring data from the frontend electronics. Although the application does not communicate with
other instances it can be made aware of the number of filewriter processes - and it's
own rank (or index) in the overall system. This lets it calculate suitable dataset offsets for writing
frames, received in the "temporal mode" (i.e. round robin between all DAQ servers).

### Commandline Interface

The following options and arguments can be given in the usual fashion where defaults can be configured
in a configuration file (`-c,--config`) - but overriden by a user on the commandline:

	bin/filewriter --help
	usage: filewriter [options]

       odin-data version: 0-5-0dls1-155-g3ca9b51-dirty

	Generic options:
	  -h [ --help ]         Print this help message
	  -c [ --config ] arg   Specify program configuration file

	Configuration options:
      -d [ --debug-level ] arg (=0)         Set the debug level
      -l [ --logconfig ] arg                Set the log4cxx logging configuration
                                            file
      --iothreads arg (=1)                  Set number of IPC channel IO threads
      --ctrl arg (=tcp://0.0.0.0:5004)      Set the control endpoint
      --ready arg (=tcp://127.0.0.1:5001)   Ready ZMQ endpoint from frameReceiver
      --release arg (=tcp://127.0.0.1:5002) Release frame ZMQ endpoint from
                                            frameReceiver
      --meta arg (=tcp://*:5558)            ZMQ meta data channel publish stream
      -I [ --init ]                         Initialise frame receiver and meta
                                            interfaces.
      -N [ --no-client ]                    Enable full initial configuration to
                                            run without any client controller.You
                                            must also be provide: detector, path,
                                            datasets, dtype and dims.
      -p [ --processes ] arg (=1)           Number of concurrent file writer
                                            processes
      -r [ --rank ] arg (=0)                The rank (index number) of the current
                                            file writer process in relation to the
                                            other concurrent ones
      --detector arg                        Detector to configure for
      --path arg                            Path to detector shared library with
                                            format 'lib<detector>ProcessPlugin.so'
      --datasets arg                        Name(s) of datasets to write (space
                                            separated)
      --dtype arg                           Data type of raw detector data (0:
                                            8bit, 1: 16bit, 2: 32bit, 3:64bit)
      --dims arg                            Dimensions of each frame (space
                                            separated)
      -C [ --chunk-dims ] arg               Chunk size of each sub-frame (space
                                            separated)
      --bit-depth arg                       Bit-depth mode of detector
      --compression arg                     Compression type of input data (0:
                                            None, 1: LZ4, 2: BSLZ4)
      -o [ --output ] arg (=test.hdf5)      Name of HDF5 file to write frames to
                                            (default: test.hdf5)
      --output-dir arg (=/tmp/)             Directory to write HDF5 file to
                                            (default: /tmp/)
      --extension arg (=h5)                 Set the file extension of the data
                                            files (default: h5)
      -S [ --single-shot ]                  Shutdown after one dataset completed
      -f [ --frames ] arg (=0)              Set the number of frames to write into
                                            dataset
      --acqid arg                           Set the Acquisition Id of the
                                            acquisition
      --timeout arg (=0)                    Set the timeout period for closing the
                                            file (milliseconds)
      --block-size arg (=1)                 Set the number of consecutive frames to
                                            write per block
      --blocks-per-file arg (=0)            Set the number of blocks to write to
                                            file. Default is 0 (unlimited)
      --earliest-hdf-ver                    Set to use earliest hdf5 file version.
                                            Default is off (use latest)
      --alignment-threshold arg (=1)        Set the hdf5 alignment threshold.
                                            Default is 1 (no alignment)
      --alignment-value arg (=1)            Set the hdf5 alignment value. Default
                                            is 1 (no alignment)
      -j [ --json_file ] arg                Path to a JSON configuration file to
                                            submit to the application

### Limitations

Currently the application has a number of limitations (which will be addressed):

 * No internal buffering: the application currently depend on the buffering available in the frameReceiver.
   If the filewriter cannot keep up the pace it will not release frames back to the frameReceiver in time,
   potentially causing the frameReceiver to run out of buffering and drop frames. Fortunately missing frames
   are obvious in the output files (as empty gaps)
 * Metadata like the "info" field and original frame ID number is not recorded in the file.
 * Sensor data is only stored in raw 16bit format for both gain and reset frames. The fine/coarse ADC and
   gain data can be generated in the file by a post-processing step (see python script
   `decode_raw_frames_hdf5.py`)


Using JSON Configuration Files
------------------------------

Both the FrameReceiver and FrameProcessor applications support the use of JSON formatted configuration files
supplied from the command line to set them up without the need for a control connection.  All configuration
messages that are supported from the control interface are also supported through the configuration file.

To supply a JSON configuration file from the command line use the `-j` or `--json_file` options and supply the
full path and filename of the configuration file.  The file must be correctly formatted JSON otherwise the
parsing will fail.

### File Structure

The file must specify a list of dictionaries, with each dictionary representing a single configuration message
that is to be submitted through the control interface.  When the configuration file is parsed each dictionary is
processed in turn and submitted as a single control message, which allows for multiple instances of the same control
message to be submitted in a specific order (something that is required when loading plugins into a FrameProcessor
application.

The example below demonstrates setting up a FrameProcessor with two plugins.  There are five control messages
defined here, with two sets of duplicated messages (for loading and connecting plugins).  The first message will
setup the interface from the FrameProcessor to the FrameReceiver application.  The second message loads the
FileWriter plugin into the FrameProcessor application.  The third message loads another plugin (ExcaliburProcessPlugin)
into the FrameProcessor application.  The final two messages connect the plugins together in a chain with the
shared memory buffer manager.

```
[
  {
    "fr_setup": {
      "fr_ready_cnxn": "tcp://127.0.0.1:5001",
      "fr_release_cnxn": "tcp://127.0.0.1:5002"
    }
  },
  {
    "plugin": {
      "load": {
        "index": "hdf",
        "name": "FileWriterPlugin",
        "library": "/dls_sw/prod/tools/RHEL6-x86_64/odin-data/0-5-0dls1/prefix/lib/libHdf5Plugin.so"
      }
    }
  },
  {
    "plugin": {
      "load": {
        "index": "excalibur",
        "name": "ExcaliburProcessPlugin",
        "library": "/dls_sw/prod/tools/RHEL6-x86_64/excalibur-detector/0-3-0/prefix/lib/libExcaliburProcessPlugin.so"
      }
    }
  },
  {
    "plugin": {
      "connect": {
        "index": "excalibur",
        "connection": "frame_receiver"
      }
    }
  },
  {
    "plugin": {
      "connect": {
        "index": "hdf",
        "connection": "excalibur"
      }
    }
  }
]
```

Using the python tools
----------------------

Several python scripts in this area can be used to emulate parts of the Percival
data acquisition system. This is a summary of the tools and instructions on how to
operate them. There are two options for using the tools:

* Create a virtual python environment, then install packages and the tools into it.
* Install required python packages and set PYTHONPATH to point at the appropriate location

The virtual environment is the preferred solution since it does not require system privileges
to install packages.

__NOTE__ The `h5py` python bindings require the HDF5 libraries to be installed on your system (see above). If
not installed into a standard location, set the environment variable `HDF5_DIR` to point to the installation,
prior to either installation method. See the [h5py install instructions](http://docs.h5py.org/en/latest/build.html)
for more information.


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

    # Move back to the "build" directory
    cd ../..


Having set up the virtual environment and installed packages and tools, it can be subsequently
reused simply by activating it with the `source venv/bin/activate` command. The virtual env can
be exited with the command `deactivate` at any time.

The `setup.py` script creates command-line entry points for the tools in the virtual environment,
which can be invoked directly, as shown in the descriptions below.

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
	pip install pysnmp
	pip install numpy
	pip install h5py
	pip install pyzmq

	# Set the PYTHONPATH to point at the location of the tools
	export PYTHONPATH=<project root>/build/lib/python

The `pyzmq` python bindings for ZeroMQ do not require ZeroMQ to be installed, although this is requried to
build and run the compiled applications. There is, however, a potential conflict between `pyzmq` and ZeroMQ v3
libraries on RedHat-based systems, causing python tools to fail when importing the bindings. If this occurs,
delete the bindings with the command `pip unintall pyzmq` and reinstall with
`pip install pyzmq --install-option='--zmq=bundled'` to force installation of a local ZeroMQ library.

__NOTE__: unlike in the virtual environment, this installation method does __not__ create command-line
entry points for the python tools. In this case, invoking them requires `python` to run with a module
entry instead. For instance `port_counters ...` becomes `python -m port_counters ...`.

Available python tools
----------------------

The following describe the available tools, and their configurations.

0
