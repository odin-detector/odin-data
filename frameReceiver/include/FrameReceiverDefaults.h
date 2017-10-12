/*
 * FrameReceiverDefaults.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERDEFAULTS_H_
#define FRAMERECEIVERDEFAULTS_H_

#include <cstddef>
#include <stdint.h>
#include <string>

namespace FrameReceiver
{
namespace Defaults
{
enum RxType
{
  RxTypeIllegal = -1,
  RxTypeUDP,
  RxTypeZMQ,
};

const int          default_node                   = 1;
const std::size_t  default_max_buffer_mem         = 1048576;
const std::string  default_decoder_type           = "unknown";
const RxType       default_rx_type                = RxTypeUDP;
const std::string  default_rx_port_list           = "8989,8990";
const std::string  default_rx_address             = "0.0.0.0";
const int          default_rx_recv_buffer_size    = 30000000;
const std::string  default_rx_chan_endpoint       = "inproc://rx_channel";
const std::string  default_ctrl_chan_endpoint     = "tcp://*:5000";
const std::string  default_frame_ready_endpoint   = "tcp://*:5001";
const std::string  default_frame_release_endpoint = "tcp://*:5002";
const std::string  default_shared_buffer_name     = "FrameReceiverBuffer";
const unsigned int default_frame_timeout_ms       = 1000;
const unsigned int default_frame_count            = 0;
const bool         default_enable_packet_logging  = false;
const bool         default_force_reconfig         = false;

}

} // namespace FrameReceiver
#endif /* FRAMERECEIVERDEFAULTS_H_ */
