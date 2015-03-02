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

        enum FrameReceiveState  { FrameReceiveStateEmpty, FrameReceiveStateIncomplete, FrameReceiveStateComplete };
        enum PacketReceiveState { PacketReceiveStateHeader, PacketReceiveStatePayload };

        FrameDecoder(LoggerPtr& logger) :
            logger_(logger),
            frame_receive_state_(FrameReceiveStateEmpty),
            packet_receive_state_(PacketReceiveStateHeader)
        {};
        virtual ~FrameDecoder() = 0;

        void register_buffer_manager(SharedBufferManagerPtr buffer_manager)
        {
            buffer_manager_ = buffer_manager;
        }

        virtual const size_t get_frame_buffer_size(void) const = 0;
        virtual const size_t get_frame_header_size(void) const = 0;
        virtual const size_t get_packet_header_size(void) const = 0;

        virtual const bool requires_header_peek(void) const = 0;

        virtual void* get_next_receive_location(void) const = 0;
        virtual size_t get_next_receive_size(void) const = 0;

        virtual void process_received_data(size_t bytes_received) = 0;

        virtual boost::shared_ptr<void> get_packet_header_buffer(void) = 0;

        FrameReceiveState get_frame_receive_state(void) const
        {
            return frame_receive_state_;
        }

        void set_frame_receive_state(FrameReceiveState state)
        {
            frame_receive_state_ = state;
        }

        PacketReceiveState get_packet_receive_state(void) const
        {
            return packet_receive_state_;
        }

        void set_packet_receive_state(PacketReceiveState state)
        {
            packet_receive_state_ = state;
        }

    protected:
        LoggerPtr logger_;
        FrameReceiveState  frame_receive_state_;
        PacketReceiveState packet_receive_state_;
        SharedBufferManagerPtr buffer_manager_;
    };

    inline FrameDecoder::~FrameDecoder() {};

    typedef boost::shared_ptr<FrameDecoder> FrameDecoderPtr;
}
#endif /* INCLUDE_FRAMEDECODER_H_ */
