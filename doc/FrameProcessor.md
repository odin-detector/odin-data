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
other instances it can be made aware of the number of frameProcessor applications and it's
own rank (or index) in the overall system. This lets it calculate suitable dataset offsets for writing
frames, received in the "temporal mode" (i.e. round robin between all DAQ servers).

### Commandline Interface

The following options and arguments can be given on the commandline:

    $ frameProcessor --version
    frameProcessor version 1-3-0dls3-1-g4c39452-dirty


    $ frameProcessor --help
    Usage: frameProcessor [options]

    Generic options:
      -h [ --help ]         Print this help message
      -v [ --version ]      Print program version string
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
      -j [ --json_file ] arg                Path to a JSON configuration file to
                                            submit to the application

### Limitations

Currently the application has a number of limitations (which will be addressed):

 * No internal buffering: the application currently depend on the buffering available in the frameReceiver.
   If the frameProcessor cannot keep up the pace it will not release frames back to the frameReceiver in time,
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

### Configuration Options

The IPC control channels operate via IPCMessages. There are a few main message types:

###### Shutdown - Cleanly shutdown application
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdShutdown`

###### Reset Statistics - Reset any parameters storing long running statistics
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdResetStatistics`

###### Version Request - Request version parameters
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdRequestVersion`

###### Status Request - Request current read-only status parameters
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdStatus`

###### Configuration Request - Request read-write configuration parameters
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdRequestConfiguration`

###### Set Configuration - Set the value of a read/write parameter
`MsgType = MsgTypeCommand, MsgVal = MsgValCmdConfigure`

#### FrameProcessorController Configuration

##### Load Plugin
```json
{
  "plugin": {
    "load": {
      "index": "<INDEX>",
      "name": "<CLASS NAME>",
      "library": "<PATH TO LIBRARY>"
    }
  }
}
```

__NOTE__: <INDEX> is purely a label for referencing the plugin in further commands

##### Connect Plugins
```json
{
  "plugin": {
    "connect": {
      "index": "<INDEX>",
      "connection": "<INDEX TO CONNECT TO>"
    }
  }
}
```

##### Disconnect Plugins
```json
{
  "plugin": {
    "disconnect": {
      "index": "<INDEX>",
      "connection": "<INDEX TO DISCONNECT FROM>"
    }
  }
}
```

##### Disconnect All Plugins
```json
{
  "plugin": {
    "disconnect": "all"
  }
}
```

##### Plugin Configuration
```json
{
  "plugin": {
    "<INDEX>": {
      <PLUGIN CONFIG>
    }
  }
}
```

##### Set Debug Level
```json
{
  "debug": <DEBUG LEVEL>
}
```

##### Clear Errors
```json
{
  "clear_errors": (Value Ignored)
}
```

##### Store Config
```json
{
  "store": {
    "index": <CONFIG INDEX>,
    "value": <JSON CONFIG>
  }
}
```

##### Execute Config
```json
{
  "execute": {
    "index": <CONFIG INDEX>
  }
}
```

Config store is used to save a JSON config entries to be executed at a later time. This is useful for
swapping between plugin chains or different operating modes with sets of parameter values.
Both the JSON config file and the store config can be a JSON formatted list of multiple config dictionaries.

#### FileWriterPlugin Config

##### Process Config
```json
{
  "process": {
    "number": <TOTAL NUMBER OF PROCESSES>,
    "rank": <RANK OF THIS PROCESS>
  }
}
```

__NOTE__: Rank is zero-indexed

##### File Config
```json
{
  "file": {
    "name": <NAME OF FILE>,
    "path": <PATH TO FILE DIRECTORY>,
    "extension": <EXTENSION OF FILE>
  }
}
```

__NOTE__: Full file path will be {path}{name}{number}{extension} where number is generated by the process based
on its rank, the total number of processes and how many files it has written.

##### Create Dataset
```json
{
  "dataset": <DATASET NAME>
}
```

__NOTE__: Dataset \<DATASET NAME> will be created the first time a dataset config is sent with it included.

##### Dataset Config
```json
{
  "dataset": {
    <DATASET NAME>: {
       "datatype": <DATATYPE>,
       "dims": <IMAGE DIMENSIONS ARRAY>,
       "chunks": <IMAGE CHUNKING DIMENSIONS ARRAY>
       "compression": <COMPRESSION MODE>
    }
  }
}
```

Allowed \<DATATYPE>: "uint8", "uint16", "uint32", "uint64", "float"

Allowed \<COMPRESSION MODE>: "none", "LZ4", "BSLZ4", "blosc"

##### Dataset Blosc Config
```json
{
  "dataset": {
    <DATASET NAME>: {
       "blosc_compressor": <BLOSC COMPRESSOR>,
       "blosc_level": <BLOSC LEVEL>,
       "blosc_shuffle": <BLOSC SHUFFLE MODE>
    }
  }
}
```

Allowed \<BLOSC COMPRESSOR>: 0-5 (blosclz, lz4, lz4hc, snappy, zlib, zstd)

Allowed \<BLOSC LEVEL>: 1-8

Allowed \<BLOSC SHUFFLE>: 0-2 (none, byteshuffle, bitshuffle)

##### Start/Stop Writing
```json
{
  "write": <True/False>
}
```
