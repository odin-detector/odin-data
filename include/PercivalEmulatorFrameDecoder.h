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
#include <time.h>

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

        typedef enum
        {
            PacketTypeSample = 0,
            PacketTypeReset  = 1,
        } PacketType;

        static const size_t primary_packet_size = 8192;
        static const size_t num_primary_packets = 255;
        static const size_t tail_packet_size    = 512;
        static const size_t num_tail_packets    = 1;
        static const size_t num_subframes       = 2;
        static const size_t num_data_types      = 2;

        typedef struct
        {
            uint32_t frame_number;
            uint32_t frame_state;
            struct timespec frame_start_time;
            uint32_t packets_received;
            uint8_t  packet_state[num_data_types][num_subframes][num_primary_packets + num_tail_packets];
        } FrameHeader;

        static const size_t subframe_size       = (num_primary_packets * primary_packet_size)
                + (num_tail_packets * tail_packet_size);
        static const size_t data_type_size      = subframe_size * num_subframes;
        static const size_t total_frame_size    = (data_type_size * num_data_types) + sizeof(FrameHeader);
        static const size_t num_frame_packets   = num_subframes * num_data_types *
                (num_primary_packets + num_tail_packets);

        PercivalEmulatorFrameDecoder(LoggerPtr& logger, bool ebable_packet_logging=false, unsigned int frame_timeout_ms=1000);
        ~PercivalEmulatorFrameDecoder();

        const size_t get_frame_buffer_size(void) const;
        const size_t get_frame_header_size(void) const;

        inline const bool requires_header_peek(void) const { return true; };
        const size_t get_packet_header_size(void) const;
        void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr);

        void* get_next_payload_buffer(void) const;
        size_t get_next_payload_size(void) const;
        FrameDecoder::FrameReceiveState process_packet(size_t bytes_received);

        void monitor_buffers(void);

        void* get_packet_header_buffer(void);

        uint8_t get_packet_type(void) const;
        uint8_t get_subframe_number(void) const;
        uint16_t get_packet_number(void) const;
        uint32_t get_frame_number(void) const;

    private:

        uint8_t* raw_packet_header(void) const;
        unsigned int elapsed_ms(struct timespec& start, struct timespec& end);

        boost::shared_ptr<void> current_packet_header_;
        boost::shared_ptr<void> dropped_frame_buffer_;

        uint32_t current_frame_seen_;
        int current_frame_buffer_id_;
        void* current_frame_buffer_;
        FrameHeader* current_frame_header_;

        bool dropping_frame_data_;

        unsigned int frame_timeout_ms_;
        unsigned int frames_timedout_;
    };

} // namespace FrameReceiver

#endif /* INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_ */
