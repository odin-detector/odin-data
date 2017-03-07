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
#include <netinet/in.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>

#include "OdinDataException.h"
#include "SharedBufferManager.h"

namespace FrameReceiver
{
    class FrameDecoderException : public OdinData::OdinDataException
    {
    public:
        FrameDecoderException(const std::string what) : OdinData::OdinDataException(what) { };
    };

    typedef boost::function<void(int, int)> FrameReadyCallback;

    class FrameDecoder
    {
    public:

        enum FrameReceiveState
		{
        	FrameReceiveStateEmpty,
        	FrameReceiveStateIncomplete,
			FrameReceiveStateComplete,
			FrameReceiveStateTimedout,
			FrameReceiveStateError
        };

        FrameDecoder() :
            logger_(0),
            enable_packet_logging_(false),
        	frame_timeout_ms_(1000),
			frames_timedout_(0)
        {
        };

        virtual ~FrameDecoder() = 0;

        virtual void init(LoggerPtr& logger, bool enable_packet_logging=false, unsigned int frame_timeout_ms=1000)
        {
            logger_ = logger;
            enable_packet_logging_ = enable_packet_logging;
            // Retrieve the packet logger instance
            packet_logger_ = Logger::getLogger("FR.PacketLogger");
            // Setup frame timeout
        	frame_timeout_ms_ = frame_timeout_ms;
        };

        void register_buffer_manager(OdinData::SharedBufferManagerPtr buffer_manager)
        {
            buffer_manager_ = buffer_manager;
        }

        void register_frame_ready_callback(FrameReadyCallback callback)
        {
        	ready_callback_ = callback;
        }

        virtual const size_t get_frame_buffer_size(void) const = 0;
        virtual const size_t get_frame_header_size(void) const = 0;

        virtual const bool requires_header_peek(void) const = 0;

        virtual const size_t get_packet_header_size(void) const = 0;
        virtual void* get_packet_header_buffer(void) = 0;
		virtual void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr) = 0;

        virtual void* get_next_payload_buffer(void) const = 0;
        virtual size_t get_next_payload_size(void) const = 0;
        virtual FrameReceiveState process_packet(size_t bytes_received) = 0;

        virtual void monitor_buffers(void) = 0;

        void push_empty_buffer(int buffer_id)
        {
        	empty_buffer_queue_.push(buffer_id);
        }

        const size_t get_num_empty_buffers(void) const
        {
        	return empty_buffer_queue_.size();
        }

        const size_t get_num_mapped_buffers(void) const
        {
            return frame_buffer_map_.size();
        }

    protected:
        LoggerPtr logger_;

        bool enable_packet_logging_;
        LoggerPtr packet_logger_;

        OdinData::SharedBufferManagerPtr buffer_manager_;
        FrameReadyCallback   ready_callback_;

        std::queue<int>    empty_buffer_queue_;
        std::map<uint32_t, int> frame_buffer_map_;

        unsigned int frame_timeout_ms_;
        unsigned int frames_timedout_;
    };

    inline FrameDecoder::~FrameDecoder() {};

    typedef boost::shared_ptr<FrameDecoder> FrameDecoderPtr;
}
#endif /* INCLUDE_FRAMEDECODER_H_ */
