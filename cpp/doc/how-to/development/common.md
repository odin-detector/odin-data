# Common API

The frameProcessor and frameReceiver share some common code, mainly relating to the API
between them.

## IPC Interface

The [IpcChannel](../../reference/api/common/ipc_channel) and
[IpcMessage](../../reference/api/common/ipc_message)
classes are used for all interprocess communications - both between frameReceiver and
frameProcesor as well as for the control interface for client applications. The
`IPCMessage` class has some enumerated message types for common functions.

## SharedBufferManager

The SharedBufferManager is the shared code interface used by both applications to
access the shared memory buffers. This class allows accessing buffers by a simple index,
which simplifies the communication over the ready and release channels required
manage access to the shared memory.

## Detector Constants and Buffer Header

Detector implementations should create a definitions header containing any constants,
utilities and most importantly a common frame header struct that can be used in both the
[FrameDecoder](frame_decoder) and a [FrameProcessorPlugin](frame_processor_plugin) to
read and write to buffers in shared memory. With this common API, the decoder can insert
the header in front of the data blob in shared memory and the processor plugin can
simply cast the void pointer into a pointer to a detector specific header to access the
members and offset that pointer by the size of the header to access the data.

## Dynamic Class Versioning

As dynamically loadable classes, both the `FrameDecoder` and `FrameProcessorPlugin`
inherit `IVersionedObject`, so must implement the following methods:

- [get_version_major](../../reference/api/common/iversioned_object)
- [get_version_minor](../../reference/api/common/iversioned_object)
- [get_version_patch](../../reference/api/common/iversioned_object)
- [get_version_short](../../reference/api/common/iversioned_object)
- [get_version_long](../../reference/api/common/iversioned_object)

These methods are used to provide version information to clients about the exact classes
that have been loaded into the applications.