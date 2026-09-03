# FrameReceiver

## Commandline Interface

The frameReceiver is a C++ compiled application and is configured by a combination of
command-line options and equivalent parameters stored in an INI-formatted configuration
file. The command-line options take precedence over the configuration file if specified.
The options and their default values can be viewed by invoking the `--help` option:

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

  Specify the debug level. Increasing the value increases the verbosity of the debug
  output.

* `-n` or `--node`

  Set the frame receiver node ID. Identifies the node in a multi-receiver system.

* `-l` or `--logconfig`

  Set the log4cxx logging configuration file to use, which configures the format and
  destination    of logging output from the application. See the README.md file in the
  `config` directory for more information.

* `-m` or `--maxmem`

  Set the maximum amount of shared memory to allocate for frame buffers. This memory is
  where received frames are stored and handed off for processing by e.g. the
  FrameProcessor.

* `-s` or `--sensortype`

  Set the sensor type to receive frame data from. This a string parameter describing
  which type of data the receiver should expect. Currently only a type of
  `percivalemulator` is supported.

* `-p` or `--port`

  Set the port(s) to receive frame data on, specified as a comma-separated list, e.g.
  `8989,8990`.

* `-i`  or `--ipaddress`

  Set the the IP address to listen for data on. The default value of `0.0.0.0` listens
  on all available network interfaces.

* `--sharedbuf`

  Set the name of the shared memory frame buffer to use. Needs to match the name used
  by the downstream processing task, e.g. the frameProcessor.

* `--frametimeout`

  Set the timeout in milliseconds for releasing incomplete frames (i.e. those missing
  packets) to the downtream processing task.

* `-f` or `--frames`

  Set the number of frames to receive before terminating. The frameReceiver will wait
  for those frames to be released by the processing task before terminating. The
  default value of 0 means run indefinitely.

* `--packetlog`

  Set to a non-zero value to enable logging of packet diagnostics to a separate log
  file, whose format and destination are configured in the logging configuration file.

  > WARNING: Turning this option on will produce large quantities of output and
  > significantly impact on the performance of the frameReceiver.

* `--rxbuffer`

  Set UDP receive buffer size in bytes.

An example configuration file `fr_test.config` i.s available in the `config` directory.
Typical invocation of the frameReceiver in a test would be as follows:

    bin/frameReceiver --config test_config/fr_test.config --logconfig test_config/fr_log4cxx.xml --debug 2 --frames 3
