/*
 * FrameDecoderUDP.h
 *
 *  Created on: April 20, 2017
 *      Author: Alan Greer
 */

#ifndef INCLUDE_FRAMEDECODER_UDP_H_
#define INCLUDE_FRAMEDECODER_UDP_H_

#include <queue>
#include <map>

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>

#include "FrameDecoder.h"

namespace FrameReceiver
{
    class FrameDecoderUDP : public FrameDecoder
    {
    public:

        FrameDecoderUDP() :
        	FrameDecoder()
        {
        };

        virtual ~FrameDecoderUDP() = 0;

        virtual const bool requires_header_peek(void) const = 0;

        virtual const size_t get_packet_header_size(void) const = 0;
        virtual void* get_packet_header_buffer(void) = 0;
		virtual void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr) = 0;

        virtual void* get_next_payload_buffer(void) const = 0;
        virtual size_t get_next_payload_size(void) const = 0;
        virtual FrameReceiveState process_packet(size_t bytes_received) = 0;
    };

    inline FrameDecoderUDP::~FrameDecoderUDP() {};

    typedef boost::shared_ptr<FrameDecoderUDP> FrameDecoderUDPPtr;
}
#endif /* INCLUDE_FRAMEDECODER_UDP_H_ */
