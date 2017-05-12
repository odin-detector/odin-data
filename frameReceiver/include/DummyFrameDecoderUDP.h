/*
 * DummyFrameDecoderUDP.h
 *
 *  Created on: 12 May 2107
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_DUMMYFRAMEDECODERUDP_H_
#define INCLUDE_DUMMYFRAMEDECODERUDP_H_

#include "FrameDecoderUDP.h"
#include <iostream>
#include <stdint.h>
#include <time.h>

namespace FrameReceiver
{

	class DummyFrameDecoderUDP : public FrameDecoderUDP
	{
	public:
		DummyFrameDecoderUDP();
		~DummyFrameDecoderUDP();

		void init(LoggerPtr& logger, bool enable_packet_logging=false, unsigned int frame_timeout_ms=1000);

		const size_t get_frame_buffer_size(void) const { return static_cast<const size_t>(0); };
		const size_t get_frame_header_size(void) const { return static_cast<const size_t>(0); };

		inline const bool requires_header_peek(void) const { return false; };
		const size_t get_packet_header_size(void) const { return static_cast<const size_t>(0); };
		void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr) { };

		void* get_next_payload_buffer(void) const { return static_cast<void *>(0); };
		size_t get_next_payload_size(void) const { return static_cast<const size_t>(0); };
		FrameDecoder::FrameReceiveState process_packet(size_t bytes_received);

		void monitor_buffers(void) { };

		void* get_packet_header_buffer(void){ return reinterpret_cast<void *>(0); };

	};
}

#endif /* INCLUDE_DUMMYFRAMEDECODERUDP_H_ */

