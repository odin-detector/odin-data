# FrameProcessor

## Commandline Interface

The following options and arguments can be given on the commandline:

    $ frameProcessor --version
    frameProcessor version 1.7.0

    $ frameProcessor --help
    Usage: frameProcessor [options]


    Generic options:
      -h [ --help ]                         Print this help message
      -v [ --version ]                      Print program version string

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

## Limitations

Currently the application has a number of limitations (which will be addressed):

- No internal buffering: the application currently depend on the buffering available in
  the frameReceiver. If the frameProcessor cannot keep up the pace it will not release
  frames back to the frameReceiver in time, potentially causing the frameReceiver to run
  out of buffering and drop frames. Fortunately missing frames are obvious in the output
  files (as empty gaps).
- Metadata like the "info" field and original frame ID number is not recorded in the
  file.
- Sensor data is only stored in raw 16bit format for both gain and reset frames. The
  fine/coarse ADC and gain data can be generated in the file by a post-processing step
  (see python script `decode_raw_frames_hdf5.py`)

## Runtime Configuration

Further configuration of the application is done by json configuration messages from a
client, but for convenience configuration can be passed as a JSON file on startup to set
them up without the need for a control connection. All configuration messages that are
supported from the control interface are also supported through the configuration file.

To supply a JSON configuration file from the command line use the `-j` or `--json_file`
options and supply the full path and filename of the configuration file. The file must
be correctly formatted JSON otherwise the parsing will fail.

### File Structure

The file must specify a list of dictionaries, with each dictionary representing a
single configuration message that is to be submitted through the control interface.
When the configuration file is parsed each dictionary is processed in turn and submitted
as a single control message, which allows for multiple instances of the same control
message to be submitted in a specific order (something that is required when loading
plugins into a FrameProcessor application.

The example below demonstrates setting up a FrameProcessor with two plugins. There are
five control messages defined here, with two sets of duplicated messages (for loading
and connecting plugins).

- The first message will setup the interface from the FrameProcessor to the
  FrameReceiver application
- The second message loads the DummyProcessPlugin into the FrameProcessor application
- The third message loads the FileWriterPlugin into the FrameProcessor application
- The final two messages connect the plugins together - in this example, the
  FileWriterPlugin is connected to the DummyProcessPlugin, which is connected to
  `frame_receiver`

```{note}
Note: `frame_receiver` is a special key that connects the plugin to the
`SharedMemoryController`. This is the FrameProcessor interface to the shared memory
made available by the FrameReceiver.
```

### Configuration Options

The IPC control channels operate via IPCMessages. There are a few main message types:

  - `cmd`
  - `ack`
  - `nack`
  - `notify`

`cmd` is used by clients to monitor and configure the applications, `notify` is passed
between the FrameReceiver and FrameProcessor to managed shared memory buffers and `ack`
and `nack` are used to report success and failure from other messages.

These are the message values that can be used by a client in a "cmd" message:

- `shutdown` - Cleanly shutdown application
- `reset_statistics` - Reset any parameters storing long running statistics
- `request_version` - Request version parameters
- `status` - Request current read-only status parameters
- `request_configuration` - Request read-write configuration parameters
- `configure` - Set the value of a read/write parameter

### FrameProcessorController

Below are some common configurations to send to a frameProcessor application. In these
examples, the given json is the value of `params` in the following complete control
message:

```json
{
  "msg_type": "cmd",
  "id": 1,
  "msg_val": "configure",
  "params": {},
  "timestamp": "2022-03-20T18:47:58.440432"
}
```

#### Initialise FrameReceiver Interface

``````{dropdown} FrameReceiver Interface
```
{
  "fr_setup": {
    "fr_ready_cnxn": "tcp://127.0.0.1:5001",
    "fr_release_cnxn": "tcp://127.0.0.1:5002"
  }
}
```
``````

#### Load Plugin

Load an instance of a plugin into the application. This can be be done multiple times
per class as long as the index is unique.

``````{dropdown} Load Plugin
```json
{
  "plugin": {
    "load": {
      "index": "sum",
      "name": "SumPlugin",
      "library": "prefix/lib/"
    }
  }
}
```
``````

```{note}
`index` is the label for referencing the plugin in further commands, such as connecting
it to another plugin
```

#### Connect Plugins

Connect one plugin to another. Frames `push`ed by the plugin given as `connection` will
be added to the queue of the plugin given by `index`.

``````{dropdown} Connect Plugin
```json
{
  "plugin": {
    "connect": {
      "index": "hdf",
      "connection": "sum"
    }
  }
}
```
``````

#### Disconnect Plugins

Disconnect the plugin given as `index` from the plugin given as `connection`.

``````{dropdown} Disconnect Plugin
```json
{
  "plugin": {
    "disconnect": {
      "index": "hdf",
      "connection": "sum"
    }
  }
}
```
``````

#### Disconnect All Plugins

A quick way to remove the connections between all plugins. This is useful as part of a
stored [stored config](#store-config) for reconnecting plugins in a different mode.

``````{dropdown} Disconnect All Plugins
```json
{
  "plugin": {
    "disconnect": "all"
  }
}
```
``````

#### Plugin Configuration

See specific plugins for what configurations are accepted. As an example, here is a
message to configure the `FileWriterPlugin` to start writing, i.e. the key is the
`index` of the plugin to configure and the value is a json dictionary that will be
passed to the plugin as configuration.

``````{dropdown} Configure Specific Plugin
```json
{
  "hdf": {
    "write": true
  }
}
```
``````

#### Set Debug Level

Set the verbosity of debug level log messages.

``````{dropdown} Set Debug Level
```json
{
  "debug": 1
}
```
``````

#### Clear Errors

Clear any error state.

``````{dropdown} Set Debug Level
```json
{
  "clear_errors": true
}
```
``````

```{note}
The value is ignored. Any valid json with the key `clear_errors` is sufficient.
```

#### Store Config

Store a series of configuration messages to be applied with the given `index`. This is
useful for reconnecting the plugins in a different design.

``````{dropdown} Store Plugin Connections With Sum
```json
{
  "store": {
    "index": "sum",
    "value": [
      {
        "plugin": {
          "disconnect": "all"
        }
      },
      {
        "plugin": {
          "connect": {
            "index": "dummy",
            "connection": "frame_receiver"
          }
        }
      },
      {
        "plugin": {
          "connect": {
            "index": "sum",
            "connection": "dummy"
          }
        }
      },
      {
        "plugin": {
          "connect": {
            "index": "hdf",
            "connection": "sum"
          }
        }
      }
    ]
  }
}
```
``````

``````{dropdown} Store Plugin Connections Without Sum
```json
{
  "store": {
    "index": "no_sum",
    "value": [
      {
        "plugin": {
          "disconnect": "all"
        }
      },
      {
        "plugin": {
          "connect": {
            "index": "dummy",
            "connection": "frame_receiver"
          }
        }
      },
      {
        "plugin": {
          "connect": {
            "index": "hdf",
            "connection": "dummy"
          }
        }
      }
    ]
  }
}
```
``````

#### Execute Config

Reconfigure a previously saved plugin connection config given by `index`.

``````{dropdown} Execute Stored Config
```json
{
  "execute": {
    "index": "sum"
  }
}
```
``````

### FileWriterPlugin

#### Process

Configure the rank of the node and the total number of nodes in the deployment.

``````{dropdown} Process
```json
{
  "process": {
    "number": 4,
    "rank": 0
  }
}
```
``````

#### File

Configure the output for the file.

``````{dropdown} File
```json
{
  "file": {
    "name": "test",
    "path": "/tmp",
    "extension": "h5"
  }
}
```
``````

```{note}
The full file path will be `{path}{name}{number}{extension}` where `number` is generated
by the process based on its rank, the total number of processes and how many files it
has written.
```

#### Create a Dataset

Create a dataset to be written to the file.

``````{dropdown} Create Dataset
```json
{
  "dataset": "data"
}
```
``````

```{note}
Dataset `<DATASET NAME>` will be created the first time any dataset config is sent with it included. It does not need this specific message before sending further configuration.
```


#### Dataset Config

Configure a dataset in the file.

``````{dropdown} Configure Dataset
```json
{
  "dataset": {
    "data": {
       "datatype": "uint64",
       "dims": [515, 2069],
       "chunks": [1, 515, 2069],
       "compression": "none"
    }
  }
}
```
``````

```{card} Data Types
`uint8`, `uint16`, `uint32`, `uint64`, `float`
```

```{card} Compression Modes
`none`, `LZ4`, `BSLZ4`, `blosc`
```

#### Dataset Blosc Config

Configure the blosc compression of the dataset.

``````{dropdown} Configure Dataset Blosc Compression
```json
{
  "dataset": {
    "data": {
       "blosc_compressor": "blosclz",
       "blosc_level": 8,
       "blosc_shuffle": 2
    }
  }
}
```
``````

```{card} Blosc Compressors
`0-5` -> `blosclz`, `lz4`, `lz4hc`, `snappy`, `zlib`, `zstd`
```

```{card} Blosc Levels
`1-8`
```

```{card} Blosc Shuffle Modes
`0-2` -> `none`, `byteshuffle`, `bitshuffle`
```


#### Start/Stop Writing

Start and stop file writing.

``````{dropdown} Start file writing
```json
{
  "write": true
}
```
``````

``````{dropdown} Stop file writing
```json
{
  "write": false
}
```
``````
