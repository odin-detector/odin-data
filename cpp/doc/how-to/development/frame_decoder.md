# FrameDecoder

The purpose of the `FrameDecoder` is to gather incoming packets or messages from a data
stream, perform minimal work to construct a single data blob in shared memory for the
FrameProcessor to handle further processing.

This class defines a few virtual methods to be implemented by concrete implementations.
Some are pure virtual - annotated with `*` - and some are optional. These are:

- `get_frame_buffer_size*`
- `get_frame_header_size*`
- `monitor_buffers*`
- `get_status`
- `request_configuration`
- `reset_statistics`

```{note}
See the [API Reference](../../reference/api/frame_receiver/decoder.md)
for more detail.
```

There is generic support for UDP and ZMQ data streams that a decoder plugin can be built
on top of, depending on the use case. These are `FrameDecoderUDP` and `FrameDecoderZMQ`,
each a child of `FrameDecoder`. They must be paired with the `FrameReceiverUDPRxThread`
and `FrameRecieverZMQRxThread` respectively (e.g. using `--rxtype udp`). Both extend
the base class and add further virtual methods to be implemented in a concrete decoder.

## FrameDecoderUDP
- `requires_header_peek*`
- `get_packet_header_size*`
- `get_next_payload_buffer*`
- `get_next_payload_size*`

```{note}
See the [API Reference](../../reference/api/frame_receiver/decoder.md#framedecoderudp)
for more detail.
```

## FrameDecoderZMQ
- `get_next_message_buffer*`
- `process_message*`
- `frame_meta_data*`

```{note}
See the [API Reference](../../reference/api/frame_receiver/decoder.md#framedecoderzmq)
for more detail.
```