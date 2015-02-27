/*
 * PercivalEmulatorFrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#ifndef INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_
#define INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_

#include "FrameDecoder.h"
#include <iostream>
#include <stdint.h>

namespace FrameReceiver
{
    class PercivalEmulatorFrameDecoder : public FrameDecoder
    {
    public:

//        typedef struct
//        {
//            uint8_t  packet_type;
//            uint8_t  subframe_number;
//            uint32_t frame_number;
//            uint16_t packet_number;
//            uint8_t  info[14];
//        } PacketHeader;

        typedef struct
        {
            uint8_t raw[22];
        } PacketHeader;

        typedef struct
        {
            uint32_t frame_number;
            uint32_t frame_state;
            // TODO add packet state pointers etc
        } FrameHeader;

        static const size_t primary_packet_size = 8192;
        static const size_t num_primary_packets = 255;
        static const size_t tail_packet_size    = 512;
        static const size_t num_tail_packets    = 1;
        static const size_t num_subframes       = 2;

        PercivalEmulatorFrameDecoder(LoggerPtr& logger);
        ~PercivalEmulatorFrameDecoder();

        const size_t get_frame_buffer_size(void) const;
        const size_t get_frame_header_size(void) const;
        const size_t get_packet_header_size(void) const;

        void* get_next_receive_location(void) const;
        size_t get_next_receive_size(void) const;

        void process_received_data(size_t bytes_received);

        boost::shared_ptr<void> get_packet_header_buffer(void);

        uint8_t get_packet_type(void) const;
        uint8_t get_subframe_number(void) const;
        uint16_t get_packet_number(void) const;
        uint32_t get_frame_number(void) const;

        void dump_header(void);

    private:

        uint8_t* raw_packet_header(void) const;

        boost::shared_ptr<void> current_packet_header_;
        boost::shared_ptr<void> scratch_payload_buffer_;
    };

} // namespace FrameReceiver

#endif /* INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_ */
