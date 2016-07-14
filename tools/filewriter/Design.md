File Writer Design Document
===========================

## Overview

The filewriter service is responsible for extracting data from a shared memory location, constructing meaningful data formats (frames) from the raw data, processing the frames and appending additional meta-data, and then ultimately writing the data out to HDF5 format files.

The service interfaces to the framereceiver by a ZeroMQ publish and subscribe mechansim along with shared access to a memory block (shared memory).

The service provides a standard IpcMessage control interface for submitting configuration messages (JSON format).  These messages can be used to load plugins, connect and disconnect plugins, retrieve status and configure individual plugins.  When new data is received it is wrapped in a Frame object, and then pointers to the Frame are passed to plugins through locked queues.  Plugins work in their own threads, which allows the service to release the shared memory block back to the framereceiver as soon as the raw data has been wrapped in a frame. 

## Detailed Design

### Classes

The following classes are used by the filewriter, an a brief description is provided below.  Full documentation of the classes and methods can be found in the generated documentation for the filewriter.

- ClassLoader - Generic shared library loader, used for dynamically loading plugins.
- DataBlock - Allocated memory block used to avoid reallocating memory for each new frame.
- DataBlockPool - Indexed pools of DataBlocks, manages memory.
- DummyPlugin - Example plugin that does nothing with frames
- FileWriter - Specific plugin for writing frames to HDF5 files
- FileWriterController - Controlling class of the service, accepting commands, managing plugins, shared memory.
- FileWriterPlugin - All plugins must inherit from this class, and implement the process method.
- Frame - A lightweight object that surrounds the data.  Contains a DataBlock retrieved from the pool.  This ojbect can be created and destroyed, as it doesn't allocate memory for the data.
- IFrameCallback - Any classes inheriting from this can be registered for callbacks when a new frame is available.  Shared pointers to frames are added to a locked queue and the queue is read in a separate thread.  When a new frame is available the callback method is invoked with a shared pointer to the frame.  The FileWriterPlugin class inherits from this class.
- SharedMemoryController - Controls the SharedMemoryParser class, and pushes frames to registered callback classes.
- SharedMemoryParser - Contains specific information regarding the setup of the shared memory buffer.  Copies data from shared memory into Frames.


Below is a class diagram for the filewriter.

![classes](https://github.com/percival-detector/framereceiver/blob/filewriter/tools/filewriter/doc/classes.png "Class Diagram")

### Startup

When the filewriter application is first started, it creates an instance of the FileWriterController class.  This class creates the IPC reactor thread ready to handle IPC messages.  The class is then configured with the control channel endpoint, which registers a ZeroMQ socket with the IPC reactor thread, which enables the filewriter to receive new configurations from external clients (either the Odin parallel detector framework or a test client supplied with the filewriter application).  The filewriter is now operational, but requires further configuration to be able to accept and process incoming frames.

To configure the filewriter to be able to receive frames from the framereceiver it is necessary to submit an IpcMessage with the relevant configuration information.  The IpcMessage can either be setup within the initialisation of the application (through command line startup parameters or within an ini file) or sent to the framereceiver via the control interface.  An example of an IpcMessage to setup the filewriter is presented below:

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "fr_setup": {
      "fr_release_cnxn": "tcp://127.0.0.1:5002",
      "fr_ready_cnxn": "tcp://127.0.0.1:5001",
      "fr_shared_mem": "FrameReceiverBuffer"
    }
  }
}
```

The fr_setup parameter must contain the three entries:

- fr\_release\_cnxn : The ZeroMQ endpoint for notification of released shared memory buffers.
- fr\_ready\_cnxn : The ZeroMQ endpoint for receiving notification of ready shared memory buffers.
- fr\_shared\_mem : The name of the shared memory buffer allocation (allocated by the framereceiver).

When the filewriter receives the message above, the FileWriterController class creates an instance of the SharedMemoryController and SharedMemoryParser classes.  The SharedMemoryParser take the name of the shared memory buffer as a parameter and opens the buffer ready for use within the application.  The SharedMemoryController sets up the two ZeroMQ IPC channels, registering them with the IPC reactor, and keeps a pointer to the SharedMemoryParser.  The filewriter is now ready to accept incoming frames from the framereceiver (or any other client that conforms to the Buffer Transfer API described below).

### Frame Processing

To inform the filewriter that a new frame is ready for processing an IpcMessage containing the details of the frame should be published on the IpcChannel that the filewriter is listening on.  An example message is presented below.

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "frame_ready",
  "msg_type": "notify",
  "params": {
    "frame": 1,
    "buffer_id": 3
  }
}
```

The IpcMessage must contain the two parameters:

- frame : The frame number.
- buffer\_id : The ID of the buffer within the shared memory block.

When the notification is received, the filewriter copies the frame from shared memory, and then publishes it's own notfication that the specified memory block is once again available for use.  An example response published by the filewriter is presented below.
 
```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "frame_release",
  "msg_type": "notify",
  "params": {
    "frame": 1,
    "buffer_id": 3
  }
}
```

The frame and buffer\_id parameters are identical to those received, but the msg\_val field is set to frame\_release.

The raw data is wrapped in a Frame object, which provides additional functionality such as setting the name of the data, dimensions and named parameters.  Frame objects make use of the DataBlock and DataBlockPool classes, which pre-allocate blocks of memory that can be re-used by Frames.  This avoids the need to allocate large blocks of memory when creating new Frames which can be costly.  The DataBlocks used for the raw data are separated from the Frame meta data.

Frames are passed along a plugin chain, which at a minimum contains the HDF5 writer plugin.  Frames are passed by pointer to avoid copying the entire frame, and are placed into worker queues that execute within their own threads, one per plugin.  All pointers to Frames are shared pointers, and so plugins do not need to worry about deleting any objects; when all shared pointers to a frame are destroyed the frame will be destroyed which results in the DataBlock owned by the frame returning to the DataBlockPool ready for re-use.  Using this method Frame objects are created and destroyed but the large DataBlocks that contain the actual frame data are fetched from and released to a pool.

### Plugins

Additional plugins can be loaded into the filewriter dynamically during runtime by sending the appropriate IpcMessage to the configuration channel of the filewriter.  Once loaded plugins can be placed within an existing chain or added to a new branch.  An example IpcMessage used to load the HDF5 writer plugin is presented below.

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "plugin": {
      "load": {
        "library": "./lib/libHdf5Plugin.so",
        "index": "hdf",
        "name": "FileWriter"
      }
    }
  }
}
```

The library sub-parameter should provide the path to the shared library object for dynamically loading into the filewriter application.  The index sub-parameter is a string that is used to reference the plugin within the filewriter; it must be unique for each plugin that is loaded even if the plugin is loaded multiple times.  The name sub-parameter is the name of the class to load from the library.  The example above would load the HDF5 plugin into the filewriter and assign it the index of "hdf".

Once a plugin has been loaded it can be connected to other plugins to form a chain.  The filewriter has a single reserved index for the framereceiver interface called "frame\_receiver".  If a plugin is connected to this then it will receive frames as soon as the filewriter receives a new frame from the framereceiver application.  An example of the IpcMessage required to connect the previously loaded "hdf" plugin to the "frame\_receiver" is presented below.

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "plugin": {
      "connect": {
        "index": "hdf",
        "connection": "frame_receiver"
      }
    }
  }
}
```

Plugin chains can be updated and plugins removed from the system if required, although this is unlikely to be necessary.  It is possible to send configuration messages directly to plugins through the filewriter control interface, by specifying the index of the plugin as the parameter name.  An example IpcMessage used to configure the "hdf" plugin directly is presented below.

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "hdf": {
      "dataset": {
        "cmd": "create",
        "name": "data",
        "datatype": 1,
        "dims": [1484, 1408],
        "chunks": [1, 1484, 704]
      }
    }
  }
}
```

The message above will create a new dataset definition within the HDF5 plugin.  More details of the configuration options of the HDF5 plugin can be found in the specific section below.


### Frame Recevier API

The following table describes the parameters that are published by the framereceiver and notify the filewriter that a new frame is ready for processing.


| Parameter  | Type    | Description                                                                                     |
| ---------- | ------- | ----------------------------------------------------------------------------------------------- |
| frame      | Integer | The frame number for the sequence                                                               |
| buffer_id  | Integer | The shared memory buffer ID, used by the filewriter to copy the correct data block into memory  |


The following table describes the parameters that are published by the filewriter and notify the framereceiver that a frame has been released and the shared memory block can be re-used by the framereceiver.


| Parameter  | Type    | Description                                                                                     |
| ---------- | ------- | ----------------------------------------------------------------------------------------------- |
| frame      | Integer | The frame number for the sequence (same as received)                                            |
| buffer_id  | Integer | The shared memory buffer ID (same as received)                                                  |


The filewriter will publish a response that contains the same values as it received, which will allow the framereceiver to track which frames have been successfully processed by the filewriter, and manage the shared memory buffer appropriately ensuring data is not written to a buffer that is still in use.

### Control API

The following table describes all of the parameters that are understood by the FileWriterController class.  This table does not include parameters that are specific to individual plugins, only those that apply to the filewriter application itself.


| Parameter     | Sub-parameter 1 | Sub-parameter 2 | Type    | Description                                               |
| ------------- | --------------- | --------------- | ------- | --------------------------------------------------------- |
| shutdown      |                 |                 | Boolean | Shuts down the FileWriter service and cleans up resources |
| ctrl_endpoint |                 |                 | String  | Setup the control ZeroMQ channel                          |
| status        |                 |                 | Unused  | Request status from the filewriter and all loaded plugins |
| frame         | shared_mem      |                 | String  | Name of shared memory location                            |
|               | release_cnxn    |                 | String  | ZeroMQ endpoint for the release of frames                 |
|               | ready_cnxn      |                 | String  | ZeroMQ endpoint to notify frames are ready                |
| plugin        | list            |                 | Unused  | Request a list of loaded plugin details                   |
|               | load            | name            | String  | Name of the shared library plugin to load                 |
|               |                 | index           | String  | Index of the plugin, used for referencing the plugin      |
|               |                 | library         | String  | Full path of the shared library                           |
|               | connect         | index           | String  | Index of the plugin that is being connected               |
|               |                 | connection      | String  | Index of the plugin to connect to                         |
|               | disconnect      | index           | String  | Index of the plugin that is being disconnected            |
|               |                 | connection      | String  | Index of the plugin to disconnect from                    |


To send a control message to the FileWriter application it must conform to the standard IPC message format.  See example below

```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "status": true
  }
}
```

The message must be a valid IPC message for the FileWriter to accept it.  The FileWriter expects messages with the type of "cmd"
and the value of "configure".  The message can contain any number of parameters, it is the responsibility of the FileWriter to 
reject any inconsistent sets of parameters.


### Status

Status for the FileWriter can be requested through the control API as explained above by submitting an IpcMessage with the status parameter.  When this happens the filewriter will request the current status from all loaded plugins and reply with the combined status message through the same control channel, in a standard IpcMessage format.  An example status message is presented below.


```json
{
  "timestamp": "2016-06-30T13:52:07.447634",
  "msg_val": "configure",
  "msg_type": "cmd",
  "params": {
    "excalibur": {
      "bitdepth": "12-bit"
    },
    "hdf": {
      "datasets": {
        "data": {
          "dimensions": [
            256,
            2048
          ],
          "type": 1
        }
      },
      "file_name": "test_file.h5",
      "file_path": "./",
      "frames_max": 3,
      "frames_written": 0,
      "processes": 1,
      "rank": 0,
      "writing": false
    }
  }
}
```

## File Writer HDF5 Plugin

The HDF5 plugin is provided as a core plugin available alongside the main filewriter application.  The plugin currently simply writes datasets out to HDF5 file, according to the configured dimensions and chunking.  Multiple datasets can be written to, with one single master dataset specified which controls how many frames have been considered as written.  

TODO: Consider multiple frame counters
TODO: Consider frame meta-data

The plugin can be configured using the IpcMessage control interface; submitting a configuration message that contains the hdf index for the HDF5 plugin will pass those configuration parameters to the writer.  The table below describes all of the parameters that are currently understood by the writer plugin.


| Parameter     | Sub-parameter   | Type      | Description                                               |
| ------------- | --------------- | --------- | --------------------------------------------------------- |
| process       | number          | Integer   | Number of concurrent writer processes (used for offset)   |
|               | rank            | Integer   | Rank of this writer process (used for offset)             |
| file          | path            | String    | Path of the HDF5 file to save data to                     |
|               | name            | String    | File name of the HDF5 file to save data to                |
| dataset       | cmd             | String    | create / delete (not implemented yet)                     |
|               | name            | String    | Name of dataset                                           |
|               | datatype        | Integer   | Enumeration of type, raw8bit, raw16bit, float32bit        |
|               | dims            | Integer[] | Array of dataset dimensions                               |
|               | chunks          | Integer[] | Array of dataset chunking parameters                      |
| frames        |                 | Integer   | Number of frames to write to file for next acquisition    |
| write         |                 | Boolean   | Start or stop writing frames to file                      |

