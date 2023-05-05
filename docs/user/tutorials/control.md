# Control

Both the frameReceiver and frameProcessor can, if given the appropriate starting config,
run an acquisition and shut down when it is complete. This is made possible mainly to
provide an example. The primary use case is to run the applications up in some useful
default state and then use the control interfaces to configure and runacquisitions,
interogate status and eventually shut down.

The control interfaces to the applications are ZMQ sockets, as defined by the start up
parameters. These control interfaces should be integrated into a wider application, such
as an odin-control server, or a web page for convenient control of the applications. For
debugging and development, minimal support for this is provided by the
`odin_zmq_client.py` script. For example, if we [run](run) the `test/dummy_example`
frameProcessor and frameReceiver, we can interrogate them.

## Status

We can ask the frameReceiver and frameProcessor for their entire `status`

``````{dropdown} All Status
```bash
(venv) $ python -m odin_zmq_client status
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "params": {
        "buffers": {
            "empty": 292,
            "mapped": 0,
            "total": 292
        },
        "decoder": {
            "name": "DummyUDPFrameDecoder",
            "packets_dropped": 0,
            "packets_lost": 0,
            "packets_received": 0,
            "status_get_count": 1385
        },
        "frames": {
            "dropped": 0,
            "received": 0,
            "released": 0,
            "timedout": 0
        },
        "status": {
            "buffer_manager_configured": true,
            "configuration_complete": true,
            "decoder_configured": true,
            "ipc_configured": true,
            "rx_thread_configured": true
        }
    },
    "timestamp": "2022-04-10T19:11:09.503441"
}

(venv) $ python -m odin_zmq_client status --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "params": {
        "dummy": {
            "packets_lost": 0,
            "timing": {
                "last_process": 0,
                "max_process": 0,
                "mean_process": 0
            }
        },
        "hdf": {
            "acquisition_id": "test_1",
            "file_name": "test_1_000001.h5",
            "file_path": "/tmp",
            "frames_max": 10,
            "frames_processed": 0,
            "frames_written": 0,
            "processes": 1,
            "rank": 0,
            "timeout_active": false,
            "timing": {
                "last_close": 0,
                "last_create": 638,
                "last_flush": 0,
                "last_process": 0,
                "last_write": 0,
                "max_close": 0,
                "max_create": 638,
                "max_flush": 0,
                "max_process": 0,
                "max_write": 0,
                "mean_close": 0,
                "mean_create": 638,
                "mean_flush": 0,
                "mean_process": 0,
                "mean_write": 0
            },
            "writing": true
        },
        "plugins": {
            "names": [
                "dummy",
                "hdf"
            ]
        },
        "shared_memory": {
            "configured": true
        }
    },
    "timestamp": "2022-04-10T19:12:27.639587"
}
```
``````

## Specific Status

We can ask the frameReceiver for a specific `status` parameter, e.g. the number of
dropped frames:

``````{dropdown} Frames Dropped
```bash
(venv) $ python -m odin_zmq_client status frames/dropped
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "frames/dropped": 0,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "timestamp": "2022-04-10T19:13:38.743083"
}
```
``````

Or ask the frameProcessor how many frames it has written to the current file:

``````{dropdown} Frames Written
```bash
(venv) $ python -m odin_zmq_client status hdf/frames_written --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "hdf/frames_written": 0,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "timestamp": "2022-04-10T19:15:17.187217"
}
```
``````

## Configure

We can read and write `configure` parameters:

``````{dropdown} Configure
```bash
(venv) $ python -m odin_zmq_client configure hdf/process/rank --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "request_configuration",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "hdf/process/rank": 0,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "request_configuration",
    "timestamp": "2022-04-10T19:26:44.105480"
}

(venv) $ python -m odin_zmq_client configure hdf/process/rank 2 --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "configure",
    "params": {"hdf": {"process": {"rank": 2}}},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "params": {},
    "msg_type": "ack",
    "msg_val": "configure",
    "id": 1,
    "timestamp": "2022-04-10T19:33:56.171776"
}

(venv) $ python -m odin_zmq_client configure hdf/process/rank --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "request_configuration",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "hdf/process/rank": 2,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "request_configuration",
    "timestamp": "2022-04-10T19:33:59.933050"
}
```
``````

## Commands

Some configure parameters are effectively commands, i.e. they can only be written to and
not read back. Some may have a corresponding status parameter, though. For example to
configure the FileWriterPlugin to start and stop writing:

``````{dropdown} Command
```bash
(venv) $ python -m odin_zmq_client status hdf/writing  --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "hdf/writing": true,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "timestamp": "2022-04-10T19:37:35.032422"
}

(venv) $ python -m odin_zmq_client configure hdf/write false --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "configure",
    "params": {"hdf": {"write": false}},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "params": {},
    "msg_type": "ack",
    "msg_val": "configure",
    "id": 1,
    "timestamp": "2022-04-10T19:38:27.425760"
}

(venv) $ python -m odin_zmq_client status hdf/writing  --port 5004
Request:
{
    "msg_type": "cmd",
    "id": 1,
    "msg_val": "status",
    "params": {},
    "timestamp": "2022-03-20T18:47:58.440432"
}

Response:
{
    "hdf/writing": false,
    "id": 1,
    "msg_type": "ack",
    "msg_val": "status",
    "timestamp": "2022-04-10T19:38:50.759214"
}
```
``````
