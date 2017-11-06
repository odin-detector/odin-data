/*
 * DummyFrameDecoderUDP.h
 *
 *  Created on: 12 May 2107
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_DUMMYFRAMEDECODERUDP_H_
#define INCLUDE_DUMMYFRAMEDECODERUDP_H_

#include <iostream>
#include <stdint.h>
#include <time.h>

#include "FrameDecoderUDP.h"

namespace FrameReceiver
{

  const std::string CONFIG_DECODER_UDP_PACKETS_PER_FRAME = "udp_packets_per_frame";

  namespace DummyUdpFrameDecoderDefaults
  {
    const unsigned int default_udp_packets_per_frame = 1;
  }

class DummyUDPFrameDecoder : public FrameDecoderUDP
{
public:
  DummyUDPFrameDecoder();
  ~DummyUDPFrameDecoder();

  void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);

  const size_t get_frame_buffer_size(void) const { return static_cast<const size_t>(1000000); };
  const size_t get_frame_header_size(void) const { return static_cast<const size_t>(0); };

  inline const bool requires_header_peek(void) const { return false; };
  const size_t get_packet_header_size(void) const { return static_cast<const size_t>(0); };
  void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr) { };

  void* get_next_payload_buffer(void) const { return static_cast<void *>(0); };
  size_t get_next_payload_size(void) const { return static_cast<const size_t>(0); };
  FrameDecoder::FrameReceiveState process_packet(size_t bytes_received);

  void monitor_buffers(void) { };

  void* get_packet_header_buffer(void){ return reinterpret_cast<void *>(0); };

private:

  unsigned int udp_packets_per_frame_;

};

} // namespace FrameReceiver
#endif /* INCLUDE_DUMMYFRAMEDECODERUDP_H_ */

