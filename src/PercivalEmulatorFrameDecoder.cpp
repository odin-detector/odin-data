/*
 * PercivalEmulatorFrameDecoder.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#include "PercivalEmulatorFrameDecoder.h"
#include <iostream>

using namespace FrameReceiver;

PercivalEmulatorFrameDecoder::PercivalEmulatorFrameDecoder(LoggerPtr& logger) :
        FrameDecoder(logger)
{
    current_packet_header_.reset(new uint8_t[sizeof(PercivalEmulatorFrameDecoder::PacketHeader)]);
    scratch_payload_buffer_.reset(new uint8_t[primary_packet_size]);
}

PercivalEmulatorFrameDecoder::~PercivalEmulatorFrameDecoder()
{
}

const size_t PercivalEmulatorFrameDecoder::get_frame_buffer_size(void) const
{
    const size_t partial_size = (num_primary_packets * primary_packet_size) + (num_tail_packets + tail_packet_size);
    const size_t buffer_size = (partial_size * num_subframes * 2) + get_frame_header_size();
    return buffer_size;
}

const size_t PercivalEmulatorFrameDecoder::get_frame_header_size(void) const
{
    return sizeof(PercivalEmulatorFrameDecoder::FrameHeader);
}

const size_t PercivalEmulatorFrameDecoder::get_packet_header_size(void) const
{
    return sizeof(PercivalEmulatorFrameDecoder::PacketHeader);
}

boost::shared_ptr<void> PercivalEmulatorFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header_;
}

void* PercivalEmulatorFrameDecoder::get_next_receive_location(void) const
{

    void* next_receive_location = 0;

    switch(packet_receive_state_)
    {
    case FrameDecoder::PacketReceiveStateHeader:
        next_receive_location = current_packet_header_.get();
        break;

    case FrameDecoder::PacketReceiveStatePayload:
        next_receive_location = scratch_payload_buffer_.get();
        break;

    default:
        throw FrameDecoderException("Fatal error in PercivalEmulatorFrameDecoder::get_next_receive_location: illegal frame decoder packet receive state");
        break;
    }

    return next_receive_location;
}

size_t PercivalEmulatorFrameDecoder::get_next_receive_size(void) const
{
   size_t next_receive_size = 0;

    switch(packet_receive_state_)
    {
    case FrameDecoder::PacketReceiveStateHeader:
        next_receive_size = sizeof(PercivalEmulatorFrameDecoder::PacketHeader);
        break;

    case FrameDecoder::PacketReceiveStatePayload:
        if (get_packet_number() < num_primary_packets)
        {
            next_receive_size = primary_packet_size;
        }
        else
        {
            next_receive_size = tail_packet_size;
        }
        break;

    default:
        throw FrameDecoderException("Fatal error in PercivalEmulatorFrameDecoder::get_next_receive_size: illegal frame decoder packet receive state");
        break;
    }

    return next_receive_size;
}

void PercivalEmulatorFrameDecoder::process_received_data(size_t bytes_received)
{

    switch (packet_receive_state_)
    {
    case FrameDecoder::PacketReceiveStateHeader:

        //TODO validate header, handle incoming new packet buffer allocation etc

        LOG4CXX_DEBUG(logger_, "Got packet header:"
                << " type: " << (int)get_packet_type()
                << " subframe: " << (int)get_subframe_number()
                << " packet: " << get_packet_number()
                << " frame: "  << get_frame_number()
        );
        dump_header();

        //packet_receive_state_ = FrameDecoder::PacketReceiveStatePayload;
        break;

    case FrameDecoder::PacketReceiveStatePayload:

    {
        uint8_t* payload_raw = reinterpret_cast<uint8_t*>(scratch_payload_buffer_.get());

        for (int i = 0; i < 8; i++)
        {
            int idx = i;
            std::cout << std::hex << (int)payload_raw[idx] << std::dec << " ";
        }
        std::cout << std::endl << "..." << std::endl;;
        for (int i = 0; i < 8; i++)
        {
            int idx = primary_packet_size - (i+1);
            std::cout << std::hex << (int)payload_raw[idx] << std::dec << " ";
        }
        std::cout << std::endl;
    }
        // TODO validate bytes received matches payload length
        packet_receive_state_ = FrameDecoder::PacketReceiveStateHeader;
        break;

    default:
        throw FrameDecoderException("Fatal error in PercivalEmulatorFrameDecoder::process_received_data: illegal frame decoder packet receive state");
        break;
    }

}

uint8_t PercivalEmulatorFrameDecoder::get_packet_type(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+0));
}

uint8_t PercivalEmulatorFrameDecoder::get_subframe_number(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+1));
}

uint16_t PercivalEmulatorFrameDecoder::get_packet_number(void) const
{
	uint16_t packet_number =
			(*(reinterpret_cast<uint16_t*>(raw_packet_header()+6)) << 8) |
			(*(reinterpret_cast<uint8_t*>(raw_packet_header()+7)));
    return packet_number;
}

uint32_t PercivalEmulatorFrameDecoder::get_frame_number(void) const
{
    return *(reinterpret_cast<uint32_t*>(raw_packet_header()+2));
}

void PercivalEmulatorFrameDecoder::dump_header(void)
{
    uint8_t* raw_hdr = reinterpret_cast<uint8_t*>(current_packet_header_.get());

    for (int i = 0; i < sizeof(PercivalEmulatorFrameDecoder::PacketHeader); i++)
    {
        std::cout << std::hex << (int)raw_hdr[i] << std::dec << " ";
    }
    std::cout << std::endl;
}

uint8_t* PercivalEmulatorFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_packet_header_.get());
}
