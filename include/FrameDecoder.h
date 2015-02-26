/*
 * FrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_FRAMEDECODER_H_
#define INCLUDE_FRAMEDECODER_H_

#include <stddef.h>
#include <boost/shared_ptr.hpp>

namespace FrameReceiver
{
    class FrameDecoder
    {
    public:
        virtual ~FrameDecoder() = 0;
        virtual size_t frame_buffer_size(void) const = 0;
        virtual size_t frame_header_size(void) const = 0;
        virtual size_t packet_header_size(void) const = 0;
        virtual boost::shared_ptr<void> get_packet_header_buffer(void) = 0;
    };

    inline FrameDecoder::~FrameDecoder() {};

    typedef boost::shared_ptr<FrameDecoder> FrameDecoderPtr;
}
#endif /* INCLUDE_FRAMEDECODER_H_ */
