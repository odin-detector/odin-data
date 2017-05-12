/*
 * DumyFrameDecoderUDP.cpp
 *
 *  Created on: May 12, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "DummyFrameDecoderUDP.h"

using namespace FrameReceiver;

DummyFrameDecoderUDP::DummyFrameDecoderUDP() :
	FrameDecoderUDP()
{

}

DummyFrameDecoderUDP::~DummyFrameDecoderUDP()
{
	LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP destructor");
}

void DummyFrameDecoderUDP::init(LoggerPtr& logger, bool enable_packet_logging, unsigned int frame_timeout_ms)
{
	FrameDecoderUDP::init(logger, enable_packet_logging, frame_timeout_ms);

	LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP init called");
}

FrameDecoder::FrameReceiveState DummyFrameDecoderUDP::process_packet(size_t bytes_received)
{
	LOG4CXX_TRACE(logger_, "DummyFrameDecoderUDP process_packet called");
	return FrameDecoder::FrameReceiveStateComplete;
}
