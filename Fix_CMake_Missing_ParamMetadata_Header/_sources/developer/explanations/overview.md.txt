# Overview

The design goal for odin-data is to provide a generic, high-performance, scalable data
capture and processing framework upon which specific detector data acquisition systems
can be built with dynamically loaded plugins.

The framework consists of two separate C++ applications; the frameReceiver and
frameProcessor. The frameReceiver focuses on data capture - receiving data from a
network interface into system memory buffers. The frameProcessor listens on a ZeroMQ
channel for messages from the frameReceiver containing a pointer to a data buffer in
shared memory, which it will then pass through a set of plugins, before notifying the
frameReceiver that it can re-use the shared memory.

The intended use case is to run multiple frameReceiver-frameProcessor pairs (nodes),
possibly across multiple servers, depending on the data throughput and procesing
requirements. However, an individual node does not communicate with other nodes - it is
simply made aware of the total number of nodes and its own rank in the overall system.
This lets each node verify that it is receiving the correct frame numbers (in temporal
mode), or the correct data channels (in geographical mode).

## IPC Interface

The `--ready` and `--release` arguments of the frameReceiver and frameProcessor
configure the ZeroMQ channels used to send and receive messages to manage access to
shared memory buffers.

Without an application to listen to the frameReceiver ready messages and send release
messages in response, it will exhaust the shared memory buffers and then ignore all
further data. However, the frameProcessor can be used as a diagnostic without doing
anything with the shared buffers: if no plugins are configured, the application will
still communicate with the frameReceiver and immediately releasing the shared buffers it
receives.

Where possible, the frame data transferred through a shared memory buffer is processed
in place to minimise the number of copies. However some processing requires a new memory
buffer to output to. This is a decision to be made for each individual process plugin.
See [Types of Frame] for more details.

[Types of Frame]: ../how-to/frame-processor-plugin.md#types-of-frame
