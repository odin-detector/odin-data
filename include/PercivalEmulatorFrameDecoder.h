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

        PercivalEmulatorFrameDecoder(void);
        ~PercivalEmulatorFrameDecoder();
        size_t frame_buffer_size(void) const;
        size_t frame_header_size(void) const;
        size_t packet_header_size(void) const;

        boost::shared_ptr<void> get_packet_header_buffer(void);

        uint8_t get_packet_type(void) const;
        uint8_t get_subframe_number(void) const;
        uint16_t get_packet_number(void) const;
        uint32_t get_frame_number(void) const;

        void dump_header(void);

    private:

        uint8_t* raw_packet_header(void) const;

        boost::shared_ptr<void> current_packet_header;
    };

} // namespace FrameReceiver

#endif /* INCLUDE_PERCIVALEMULATORFRAMEDECODER_H_ */
