/*
 * DummyFrameDecoderUDP.h
 *
 *  Created on: 12 May 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_DUMMYFRAMEDECODERUDP_H_
#define INCLUDE_DUMMYFRAMEDECODERUDP_H_

#include <iostream>
#include <stdint.h>
#include <time.h>

#include "FrameDecoderUDP.h"
#include "DummyUDPDefinitions.h"

namespace FrameReceiver
{

  const std::string CONFIG_DECODER_UDP_PACKETS_PER_FRAME = "udp_packets_per_frame";
  const std::string CONFIG_DECODER_UDP_PACKET_SIZE = "udp_packet_size";

  namespace DummyUdpFrameDecoderDefaults
  {
    const unsigned int default_udp_packets_per_frame = 1;
    const unsigned int default_udp_packet_size = DummyUDP::default_packet_size;
  }

class DummyUDPFrameDecoder : public FrameDecoderUDP
{
public:
  DummyUDPFrameDecoder();
  ~DummyUDPFrameDecoder();

  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();
  
  void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);
  void request_configuration(const std::string param_prefix, OdinData::IpcMessage& config_reply);

  const size_t get_frame_buffer_size(void) const;
  const size_t get_frame_header_size(void) const;

  inline const bool requires_header_peek(void) const { return true; };
  const size_t get_packet_header_size(void) const;
  void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr);

  inline const bool trailer_mode(void) const { return false; };

  void* get_next_payload_buffer(void) const;
  size_t get_next_payload_size(void) const;
  FrameDecoder::FrameReceiveState process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr);

  void monitor_buffers(void);
  void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);
  void reset_statistics(void);

  void* get_packet_header_buffer(void);

  uint32_t get_frame_number(void) const;
  uint32_t get_packet_number(void) const;

private:

  void initialise_frame_header(DummyUDP::FrameHeader* header_ptr);
    unsigned int elapsed_ms(struct timespec& start, struct timespec& end);


  unsigned int udp_packets_per_frame_;
  std::size_t udp_packet_size_;
  unsigned int status_get_count_;

  boost::shared_ptr<void> current_packet_header_;
  boost::shared_ptr<void> dropped_frame_buffer_;

  int current_frame_seen_;
  int current_frame_buffer_id_;
  void* current_frame_buffer_;
  DummyUDP::FrameHeader* current_frame_header_;
  std::size_t num_active_fems_;

  bool dropping_frame_data_;
  uint32_t packets_received_;
  uint32_t packets_lost_;
  uint32_t packets_dropped_;

};

} // namespace FrameReceiver
#endif /* INCLUDE_DUMMYFRAMEDECODERUDP_H_ */

