/*
 * PercivalEmulatorFrameDecoder.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#include "PercivalEmulatorFrameDecoder.h"
#include <iostream>

using namespace FrameReceiver;

PercivalEmulatorFrameDecoder::PercivalEmulatorFrameDecoder(void)
{
    current_packet_header.reset(new uint8_t[sizeof(PercivalEmulatorFrameDecoder::PacketHeader)]);
}

PercivalEmulatorFrameDecoder::~PercivalEmulatorFrameDecoder()
{
}

size_t PercivalEmulatorFrameDecoder::frame_buffer_size(void) const
{
    return 10000;
}

size_t PercivalEmulatorFrameDecoder::frame_header_size(void) const
{
    return 0;
}

size_t PercivalEmulatorFrameDecoder::packet_header_size(void) const
{
    return sizeof(PercivalEmulatorFrameDecoder::PacketHeader);
}

boost::shared_ptr<void> PercivalEmulatorFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header;
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
    return *(reinterpret_cast<uint16_t*>(raw_packet_header()+6));
}

uint32_t PercivalEmulatorFrameDecoder::get_frame_number(void) const
{
    return *(reinterpret_cast<uint32_t*>(raw_packet_header()+2));
}

void PercivalEmulatorFrameDecoder::dump_header(void)
{
    uint8_t* raw_hdr = reinterpret_cast<uint8_t*>(current_packet_header.get());

    for (int i = 0; i < sizeof(PercivalEmulatorFrameDecoder::PacketHeader); i++)
    {
        std::cout << std::hex << (int)raw_hdr[i] << std::dec << " ";
    }
    std::cout << std::endl;
}

uint8_t* PercivalEmulatorFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_packet_header.get());
}
