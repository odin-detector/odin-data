/*
 * DummyUDPDefinitions.h - common definitions for DummyUDP frame plugins
 * 
 * Created on: Jul 19th, 2018
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_DUMMYUDP_DEFINITIONS_H_
#define INCLUDE_DUMMYUDP_DEFINITIONS_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

namespace DummyUDP {

  // Max packet size for 9000 byte jumbo frame - (20 IPV4 + 8 UDP + 8 header)
  static const std::size_t max_packet_size = 8964;

  // Maximum packets sized for 4096*4096*2 bytes frame with 8000 byte packets
  static const std::size_t max_packets = 4195; 

  static const uint32_t start_of_frame_mask = 1 << 31;
  static const uint32_t end_of_frame_mask   = 1 << 30;
  static const uint32_t packet_number_mask   = 0x3FFFFFFF;

  static const int32_t default_frame_number = -1;
  static const std::size_t default_packet_size = 8000;

  typedef struct 
  {
    uint32_t frame_number;
    uint32_t packet_number_flags;
  } PacketHeader;

  typedef struct 
  {
    uint32_t frame_number;
    uint32_t frame_state;
    struct timespec frame_start_time;
    uint32_t total_packets_expected;
    uint32_t total_packets_received;
    std::size_t packet_size;
    uint8_t packet_state[max_packets];
  } FrameHeader;
  
  inline const std::size_t max_frame_size(void)
  {
    std::size_t max_frame_size = sizeof(FrameHeader) + (max_packet_size * max_packets);
    return max_frame_size;
  }
}

#endif /* INCLUDE_DUMMYUDP_DEFINITIONS_H_ */