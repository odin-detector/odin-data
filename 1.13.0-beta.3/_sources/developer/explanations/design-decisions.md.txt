# Design Decisions

The frameReceiver and frameProcessor are implemented as two separate applications for a
few key reasons:

## Separation of concerns

There are fundamentally two separate problem domains: data capture - receiving data from
the network interface onto the system - and data processing. These two operations can be
almost completely isolated with no coordination of the two processes other than to pass
pointers to data frames in memory. By splitting the data acquisition process into these
two parts they can be developed and debugged in isolation rather than within in a single
process with multiple threads.

By focusing one process on data capture, it is possible to optimise as required, such
as by allowing it higher priority to ensure sufficient resources. Once data is on the
system, it enters the domain of procesing and writing to persistent storage. In this
domain bottlenecks in the processing can be overcome by increasing the resources of the
processing server, such as by adding more memory, or by optimising the logic of an
isolated FrameProcessorPlugin.

## Monitoring

The first failure point in the data acquisition pipeline is receiving data from the
network interface into system memory. If this is not done quickly enough, packets will
be dropped at the kernel layer with no indication of what went wrong. Allowing the
frameReceiver to do the minimum work to gather data into memory means this usage can be
monitored and when it is exhausted it will be clear where data was lost. This monitoring
extends throughout the frameProcessor through processing time metrics for each plugin
and watchdog timers around certain I/O to show when a call is blocking for longer than
expected.
