/*
 * DummyUDPFrameDecoder.cpp
 *
 *  Created on: May 12, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "../include/DummyUDPFrameDecoder.h"
#include "version.h"

using namespace FrameReceiver;

DummyUDPFrameDecoder::DummyUDPFrameDecoder() :
    FrameDecoderUDP(),
    udp_packets_per_frame_(DummyUdpFrameDecoderDefaults::default_udp_packets_per_frame)
{

  this->logger_ = Logger::getLogger("FR.DummyDecoderPlugin");
  LOG4CXX_INFO(logger_, "DummyFrameDecoderUDP version " << this->get_version_long() << " loaded");
}

DummyUDPFrameDecoder::~DummyUDPFrameDecoder()
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP destructor");
}

int DummyUDPFrameDecoder::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int DummyUDPFrameDecoder::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int DummyUDPFrameDecoder::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string DummyUDPFrameDecoder::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string DummyUDPFrameDecoder::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

void DummyUDPFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
  FrameDecoderUDP::init(logger_, config_msg);

  udp_packets_per_frame_ = config_msg.get_param<unsigned int>(
      CONFIG_DECODER_UDP_PACKETS_PER_FRAME, udp_packets_per_frame_);

  LOG4CXX_DEBUG_LEVEL(3, logger_, "DummyUDPFrameDecoder initialised with "
      << udp_packets_per_frame_ << " UDP packets per frame");

  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP init called");
}

FrameDecoder::FrameReceiveState DummyUDPFrameDecoder::process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP process_packet called");
  return FrameDecoder::FrameReceiveStateComplete;
}

void DummyUDPFrameDecoder::get_status(const std::string param_prefix,
    OdinData::IpcMessage& status_msg)
{
  status_msg.set_param(param_prefix + "name", std::string("DummyUDPFrameDecoder"));
}
