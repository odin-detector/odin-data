# FrameDecoder

The purpose of the [FrameDecoder] is to gather incoming packets or messages from a data
stream, perform minimal work to construct a single data blob in shared memory for the
FrameProcessor to handle further processing.

This class defines a few virtual methods to be implemented by concrete implementations.
Some are pure virtual - annotated with `*` - and some are optional. These are:

- [get_frame_buffer_size](FrameReceiver::FrameDecoder::get_frame_buffer_size)*
- [get_frame_header_size](FrameReceiver::FrameDecoder::get_frame_header_size)*
- [monitor_buffers](FrameReceiver::FrameDecoder::monitor_buffers)*
- [get_status](FrameReceiver::FrameDecoder::get_status)
- [request_configuration](FrameReceiver::FrameDecoder::request_configuration)
- [reset_statistics](FrameReceiver::FrameDecoder::reset_statistics)

There is generic support for UDP and ZMQ data streams that a decoder plugin can be built
on top of, depending on the use case. These are [FrameDecoderUDP] and [FrameDecoderZMQ],
each a child of [FrameDecoder]. They must be paired with the [FrameReceiverUDPRxThread]
and [FrameReceiverUDPRxThread] respectively (e.g. setting the `rx_type` config to `udp`).
Both extend the base class and add further virtual methods to be implemented in a
concrete decoder.

## FrameDecoderUDP
- [requires_header_peek](FrameReceiver::FrameDecoderUDP::requires_header_peek)*
- [get_packet_header_size](FrameReceiver::FrameDecoderUDP::get_packet_header_size)*
- [get_next_payload_buffer](FrameReceiver::FrameDecoderUDP::get_next_payload_buffer)*
- [get_next_payload_size](FrameReceiver::FrameDecoderUDP::get_next_payload_size)*

## FrameDecoderZMQ
- [get_next_message_buffer](FrameReceiver::FrameDecoderZMQ::get_next_message_buffer)*
- [process_message](FrameReceiver::FrameDecoderZMQ::process_message)*
- [frame_meta_data](FrameReceiver::FrameDecoderZMQ::frame_meta_data)*

% Links
[FrameDecoder]: FrameReceiver::FrameDecoder
[FrameDecoderUDP]: FrameReceiver::FrameDecoderUDP
[FrameDecoderZMQ]: FrameReceiver::FrameDecoderZMQ
[FrameReceiverUDPRxThread]: FrameReceiver::FrameReceiverUDPRxThread
[FrameRecieverZMQRxThread]: FrameReceiver::FrameRecieverZMQRxThread
