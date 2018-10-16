/*
 * DummyUDPFrameDecoder.cpp
 *
 *  Created on: May 12, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "../include/DummyUDPFrameDecoder.h"

using namespace FrameReceiver;

DummyUDPFrameDecoder::DummyUDPFrameDecoder() :
    FrameDecoderUDP(),
    udp_packets_per_frame_(DummyUdpFrameDecoderDefaults::default_udp_packets_per_frame),
    status_get_count_(0)
{

}

DummyUDPFrameDecoder::~DummyUDPFrameDecoder()
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP destructor");
}

void DummyUDPFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
  FrameDecoderUDP::init(logger,config_msg);

  udp_packets_per_frame_ = config_msg.get_param<unsigned int>(
      CONFIG_DECODER_UDP_PACKETS_PER_FRAME, udp_packets_per_frame_);

  LOG4CXX_DEBUG_LEVEL(3, logger_, "DummyUDPFrameDecoder initialised with "
      << udp_packets_per_frame_ << " UDP packets per frame");

  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP init called");
}

FrameDecoder::FrameReceiveState DummyUDPFrameDecoder::process_packet(size_t bytes_received)
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP process_packet called");
  return FrameDecoder::FrameReceiveStateComplete;
}

void DummyUDPFrameDecoder::get_status(const std::string param_prefix,
    OdinData::IpcMessage& status_msg)
{
  status_get_count_++;
  status_msg.set_param(param_prefix + "name", std::string("DummyUDPFrameDecoder"));
  status_msg.set_param(param_prefix + "status_get_count", status_get_count_);
}

void DummyUDPFrameDecoder::reset_statistics()
{
  FrameDecoderUDP::reset_statistics();
  LOG4CXX_DEBUG_LEVEL(1, logger_, "DummyUDPFrameDecoder resetting statistics");
  status_get_count_ = 0;
}
