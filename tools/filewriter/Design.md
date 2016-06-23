File Writer Design Document
===========================

## Overview

The filewriter service is responsible for extracting data from a shared memory location, constructing meaningful data formats (frames) from the raw data, processing the frames and appending additional meta-data, and then ultimately writing the data out to HDF5 format files.

The service interfaces to the framereceiver by a ZeroMQ publish and subscribe mechansim along with shared access to a memory block (shared memory).

## Operations

The filewriter service subscribes to a dedicate channel to recevie notifications that a new frame is ready.  When a new notification is received, the filewriter copies the frame from shared memory, and then publishes it's own notfication that the specified memory block is once again available for use.

The frames are passed along a plugin chain, which at a minimum contains the HDF5 writer plugin.  Additional plugins can be loaded dynamically during runtime, and placed within the existing chain or creating a new branch.  Each plugin runs within its own thread, and shared pointers to the frames are passed using a locked queue.

The service can be configured using a public socket, and by passing configuration messages in the form or serialised JSON objects.  The service will forward any specific messages to plugins if the name of the plugin is supplied within the configuration object (see details below).

## Interface Control Definitions

The filewriter service understands standard IPC messages as defined within the IpcMessage class.  There are three IPC channels used by the filewriter service, and each is detailed below.  These interface 

## Class Design

- ClassLoader - Generic shared library loader, used for dynamically loading plugins.
- DataBlock - Allocated memory block used to avoid reallocating memory all of the time.
- DataBlockPool - Indexed pools of DataBlocks, manages memory.
- DummyPlugin - Example plugin that does nothing with frames
- FileWriter - Specific plugin for writing frames to HDF5 files
- FileWriterController - Controlling class of the service, accepting commands, managing plugins, shared memory.
- FileWriterPlugin - All plugins must inherit from this class, and inplement the process method
- Frame - A lightweight object that surrounds the data.  Contains a DataBlock retrieved from the pool.  This ojbect can be created and destroyed, as it doesn't allocate memory for the data.
- IFrameCallback - Any classes inheriting from this can be registered for callbacks when a new frame is available.  Shared pointers to frames are added to a locked queue and the queue is read in a separate thread.  when a new frame is available the callback method is invoked with a shared pointer to the frame.
- SharedMemoryController - Controls the SharedMemoryParser class, and pushes frames to registered callback classes.
- SharedMemoryParser - Contains specific information regarding the setup of the shared memory buffer.  Copies data from shared memory into Frames.

## Frame Recevier API


## Control API

The following table describes all of the parameters that are understood by the FileWriterController class.


| Parameter  | Sub-parameter 1 | Sub-parameter 2 | Type    | Description                                               |
| ---------- | --------------- | --------------- | ------- | --------------------------------------------------------- |
| shutdown   |                 |                 | Boolean | Shuts down the FileWriter service and cleans up resources |
| frame      | shared_mem      |                 | String  | Name of shared memory location                            |
|            | release_cnxn    |                 | String  | ZeroMQ endpoint for the release of frames                 |
|            | ready_cnxn      |                 | String  | ZeroMQ endpoint to notify frames are ready                |
| plugin     | list            |                 | Unused  | Request a list of loaded plugin details                   |
|            | load            | name            | String  | Name of the shared library plugin to load                 |
|            |                 | index           | String  | Index of the plugin, used for referencing the plugin      |
|            |                 | library         | String  | Full path of the shared library                           |
|            | connect         | index           | String  | Index of the plugin that is being connected               |
|            |                 | connection      | String  | Index of the plugin to connect to                         |
|            | disconnect      | index           | String  | Index of the plugin that is being disconnected            |
|            |                 | connection      | String  | Index of the plugin to disconnect from                    |



Messages are passed to the filewriter using JSON objects, from ZeroMQ sockets.  The filewriter subscribes to two sockets, one is dedicated to the framereceiver service, and the other is for client side control of the filewriter.  The filewriter also publishes to another ZeroMQ socket, again dedicated to the framereceiver service.  The filewriter publishes a JSON message to notify when it has finished with a frame.
 