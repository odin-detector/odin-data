/*
 * ExcaliburEmulatorFrameDecoder.h
 *
 *  Created on: Jan 16, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_EXCALIBURFRAMEDECODER_H_
#define INCLUDE_EXCALIBURFRAMEDECODER_H_

#include "FrameDecoder.h"
#include "ExcaliburDefinitions.h"
#include <iostream>
#include <stdint.h>
#include <time.h>


namespace FrameReceiver
{
    class ExcaliburFrameDecoder : public FrameDecoder
    {
    public:

        ExcaliburFrameDecoder(LoggerPtr& logger, bool ebable_packet_logging=false, unsigned int frame_timeout_ms=1000);
        ~ExcaliburFrameDecoder();

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


        uint32_t get_subframe_counter(void) const;
        uint32_t get_packet_number(void) const;
        bool get_start_of_frame_marker(void) const;
        bool get_end_of_frame_marker(void) const;

    private:

        unsigned int elapsed_ms(struct timespec& start, struct timespec& end);

        boost::shared_ptr<void> current_packet_header_;
        boost::shared_ptr<void> dropped_frame_buffer_;

        uint32_t current_frame_seen_;
        int current_frame_buffer_id_;
        void* current_frame_buffer_;
        Excalibur::FrameHeader* current_frame_header_;

        bool dropping_frame_data_;

        unsigned int frame_timeout_ms_;
        unsigned int frames_timedout_;
    };

} // namespace FrameReceiver

#endif /* INCLUDE_EXCALIBURFRAMEDECODER_H_ */
