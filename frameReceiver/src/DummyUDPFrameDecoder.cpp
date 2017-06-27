/*
 * DummyUDPFrameDecoder.cpp
 *
 *  Created on: May 12, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "../include/DummyUDPFrameDecoder.h"

using namespace FrameReceiver;

DummyUDPFrameDecoder::DummyUDPFrameDecoder() :
    FrameDecoderUDP()
{

}

DummyUDPFrameDecoder::~DummyUDPFrameDecoder()
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP destructor");
}

void DummyUDPFrameDecoder::init(LoggerPtr& logger, bool enable_packet_logging, unsigned int frame_timeout_ms)
{
  FrameDecoderUDP::init(logger, enable_packet_logging, frame_timeout_ms);

  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP init called");
}

FrameDecoder::FrameReceiveState DummyUDPFrameDecoder::process_packet(size_t bytes_received)
{
  LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP process_packet called");
  return FrameDecoder::FrameReceiveStateComplete;
}
