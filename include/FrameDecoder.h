/*
 * FrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_FRAMEDECODER_H_
#define INCLUDE_FRAMEDECODER_H_

#include <queue>
#include <map>

#include <stddef.h>
#include <stdint.h>

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameReceiverException.h"
#include "SharedBufferManager.h"

namespace FrameReceiver
{
    class FrameDecoderException : public FrameReceiverException
    {
    public:
        FrameDecoderException(const std::string what) : FrameReceiverException(what) { };
    };

    class FrameDecoder
    {
    public:

        enum FrameReceiveState
		{
        	FrameReceiveStateEmpty = 0,
        	FrameReceiveStateIncomplete,
			FrameReceiveStateComplete,
			FrameStateTimedout,
			FrameStateError
        };

        FrameDecoder(LoggerPtr& logger) :
            logger_(logger)
        {};
        virtual ~FrameDecoder() = 0;

        void register_buffer_manager(SharedBufferManagerPtr buffer_manager)
        {
            buffer_manager_ = buffer_manager;
        }

        virtual const size_t get_frame_buffer_size(void) const = 0;
        virtual const size_t get_frame_header_size(void) const = 0;

        virtual const bool requires_header_peek(void) const = 0;

        virtual const size_t get_packet_header_size(void) const = 0;
        virtual void* get_packet_header_buffer(void) = 0;
		virtual void process_packet_header(size_t bytes_received) = 0;

        virtual void* get_next_payload_buffer(void) const = 0;
        virtual size_t get_next_payload_size(void) const = 0;
        virtual FrameReceiveState process_packet(size_t bytes_received) = 0;

        void push_empty_buffer(int buffer_id)
        {
        	empty_buffer_queue_.push(buffer_id);
        }

        const size_t get_num_empty_buffers(void) const
        {
        	return empty_buffer_queue_.size();
        }

    protected:
        LoggerPtr logger_;
        SharedBufferManagerPtr buffer_manager_;

        std::queue<int>    empty_buffer_queue_;
        std::map<uint32_t, int> frame_buffer_map_;
    };

    inline FrameDecoder::~FrameDecoder() {};

    typedef boost::shared_ptr<FrameDecoder> FrameDecoderPtr;
}
#endif /* INCLUDE_FRAMEDECODER_H_ */
