# Using the Python Tools

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


## Installing a Virtual Python Environment

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

## Installing the required python packages manually

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

## Available python tools

The following describe the available tools, and their configurations.

**frame_producer**

This tool generates a simulated UDP packet stream, allowing the FrameReceiver to tested without
mezzanine hardware.

	(venv) $ frame_producer --help

	usage: frame_producer [-h] [--destaddr [DESTADDR [DESTADDR ...]]]
	                      [--frames FRAMES] [--interval INTERVAL] [--display]

	FrameProducer - generate a simulated UDP frame data stream

	optional arguments:
	  -h, --help            show this help message and exit
	  --destaddr [DESTADDR [DESTADDR ...]]
	                        list destination host(s) IP address:port (e.g.
	                        0.0.0.1:8081)
	  --frames FRAMES, -n FRAMES
	                        select number of frames to transmit
	  --interval INTERVAL, -t INTERVAL
	                        select frame interval in seconds
	  --display, -d         Enable diagnostic display of generated image

__NOTE__: multiple destination address are supported.

For example, to transmit three frames total to frame receivers on three separate hosts:

	(venv) $ frame_producer --destaddr 10.1.0.1:8000 10.1.0.2:8000 10.1.0.3:8000 --frames=3 --interval=0.1

	Starting Percival data transmission to: ['10.1.0.1', '10.1.0.2', '10.1.0.3']
	frame:  0
	  Sent Image frame: 0 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.1:8000
	  Sent Image frame: 0 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.1:8000
	  Sent Reset frame: 0 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.1:8001
	  Sent Reset frame: 0 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.1:8001
	frame:  1
	  Sent Image frame: 1 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.2:8000
	  Sent Image frame: 1 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.2:8000
	  Sent Reset frame: 1 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.2:8001
	  Sent Reset frame: 1 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.2:8001
	frame:  2
	  Sent Image frame: 2 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.3:8000
	  Sent Image frame: 2 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.3:8000
	  Sent Reset frame: 2 subframe: 0 packets: 424 bytes: 2112368  to 10.1.0.3:8001
	  Sent Reset frame: 2 subframe: 1 packets: 424 bytes: 2112368  to 10.1.0.3:8001
	3 frames completed, 25348416 bytes sent in 0.601 secs
