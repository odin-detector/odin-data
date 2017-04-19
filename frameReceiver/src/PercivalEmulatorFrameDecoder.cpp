/*
 * PercivalEmulatorFrameDecoder.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: tcn45
 */

#include "PercivalEmulatorFrameDecoder.h"
#include "gettime.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h>

using namespace FrameReceiver;

PercivalEmulatorFrameDecoder::PercivalEmulatorFrameDecoder() :
        FrameDecoder(),
		current_frame_seen_(-1),
		current_frame_buffer_id_(-1),
		current_frame_buffer_(0),
		current_frame_header_(0),
		dropping_frame_data_(false)
{
    current_packet_header_.reset(new uint8_t[sizeof(PercivalEmulator::PacketHeader)]);
    dropped_frame_buffer_.reset(new uint8_t[PercivalEmulator::total_frame_size]);
}

PercivalEmulatorFrameDecoder::~PercivalEmulatorFrameDecoder()
{
}

void PercivalEmulatorFrameDecoder::init(LoggerPtr& logger, bool enable_packet_logging, unsigned int frame_timeout_ms)
{
	FrameDecoder::init(logger, enable_packet_logging, frame_timeout_ms);

    if (enable_packet_logging_) {
        LOG4CXX_INFO(packet_logger_, "PktHdr: SourceAddress");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               SourcePort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     DestinationPort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      PacketType [1 Byte]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |  SubframeNumber [1 Byte]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |  |  FrameNumber [4 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |  |  |           PacketNumber [2 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |  |  |           |       Info [14 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |  |  |           |       |");
    }
}

const size_t PercivalEmulatorFrameDecoder::get_frame_buffer_size(void) const
{
    return PercivalEmulator::total_frame_size;
}

const size_t PercivalEmulatorFrameDecoder::get_frame_header_size(void) const
{
    return sizeof(PercivalEmulator::FrameHeader);
}

const size_t PercivalEmulatorFrameDecoder::get_packet_header_size(void) const
{
    return sizeof(PercivalEmulator::PacketHeader);
}

void* PercivalEmulatorFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header_.get();
}

void PercivalEmulatorFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    //TODO validate header size and content, handle incoming new packet buffer allocation etc

    // Dump raw header if packet logging enabled
    if (enable_packet_logging_)
    {
        std::stringstream ss;
        uint8_t* hdr_ptr = raw_packet_header();
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex;
        for (unsigned int hdr_byte = 0; hdr_byte < sizeof(PercivalEmulator::PacketHeader); hdr_byte++)
        {
            if (hdr_byte % 8 == 0) {
                ss << "  ";
            }
            ss << std::setw(2) << std::setfill('0') << (unsigned int)*hdr_ptr << " ";
            hdr_ptr++;
        }
        ss << std::dec;
        LOG4CXX_INFO(packet_logger_, ss.str());
    }

	uint32_t frame = get_frame_number();
	uint16_t packet_number = get_packet_number();
	uint8_t  subframe = get_subframe_number();
	uint8_t  type = get_packet_type();

#ifndef P2M_EMULATOR_NEW_FIRMWARE
	// Emulator firmware increments the frame number between sample and reset subframes, so as a
	// workaround to allow matching to occur, increment frame number for sample packets
	if (type == PacketTypeSample)
	{
	    frame += 1;
	}
#endif

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Got packet header:"
            << " type: "     << (int)type << " subframe: " << (int)subframe
            << " packet: "   << packet_number    << " frame: "    << frame
    );

    if (frame != current_frame_seen_)
    {
        current_frame_seen_ = frame;

    	if (frame_buffer_map_.count(current_frame_seen_) == 0)
    	{
    	    if (empty_buffer_queue_.empty())
            {
                current_frame_buffer_ = dropped_frame_buffer_.get();

    	        if (!dropping_frame_data_)
                {
                    LOG4CXX_ERROR(logger_, "First packet from frame " << current_frame_seen_ << " detected but no free buffers available. Dropping packet data for this frame");
                    dropping_frame_data_ = true;
                }
            }
    	    else
    	    {

                current_frame_buffer_id_ = empty_buffer_queue_.front();
                empty_buffer_queue_.pop();
                frame_buffer_map_[current_frame_seen_] = current_frame_buffer_id_;
                current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);

                if (!dropping_frame_data_)
                {
                    LOG4CXX_DEBUG_LEVEL(2, logger_, "First packet from frame " << current_frame_seen_ << " detected, allocating frame buffer ID " << current_frame_buffer_id_);
                }
                else
                {
                    dropping_frame_data_ = false;
                    LOG4CXX_DEBUG_LEVEL(2, logger_, "Free buffer now available for frame " << current_frame_seen_ << ", allocating frame buffer ID " << current_frame_buffer_id_);
                }
    	    }

    	    // Initialise frame header
            current_frame_header_ = reinterpret_cast<PercivalEmulator::FrameHeader*>(current_frame_buffer_);
            current_frame_header_->frame_number = current_frame_seen_;
            current_frame_header_->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
            current_frame_header_->packets_received = 0;
            memset(current_frame_header_->packet_state, 0, PercivalEmulator::num_frame_packets);
            memcpy(current_frame_header_->frame_info, get_frame_info(), PercivalEmulator::frame_info_size);
            gettime(reinterpret_cast<struct timespec*>(&(current_frame_header_->frame_start_time)));

    	}
    	else
    	{
    		current_frame_buffer_id_ = frame_buffer_map_[current_frame_seen_];
        	current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
        	current_frame_header_ = reinterpret_cast<PercivalEmulator::FrameHeader*>(current_frame_buffer_);
    	}

    }

    // Update packet_number state map in frame header
    current_frame_header_->packet_state[type][subframe][packet_number] = 1;

}

void* PercivalEmulatorFrameDecoder::get_next_payload_buffer(void) const
{

    uint8_t* next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_) +
            get_frame_header_size() +
            (PercivalEmulator::data_type_size * get_packet_type()) +
            (PercivalEmulator::subframe_size * get_subframe_number()) +
            (PercivalEmulator::primary_packet_size * get_packet_number());

    return reinterpret_cast<void*>(next_receive_location);
}

size_t PercivalEmulatorFrameDecoder::get_next_payload_size(void) const
{
   size_t next_receive_size = 0;

	if (get_packet_number() < PercivalEmulator::num_primary_packets)
	{
		next_receive_size = PercivalEmulator::primary_packet_size;
	}
	else
	{
		next_receive_size = PercivalEmulator::tail_packet_size;
	}

    return next_receive_size;
}

FrameDecoder::FrameReceiveState PercivalEmulatorFrameDecoder::process_packet(size_t bytes_received)
{

    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

	current_frame_header_->packets_received++;

	if (current_frame_header_->packets_received == PercivalEmulator::num_frame_packets)
	{

	    // Set frame state accordingly
		frame_state = FrameDecoder::FrameReceiveStateComplete;

		// Complete frame header
		current_frame_header_->frame_state = frame_state;

		if (!dropping_frame_data_)
		{
			// Erase frame from buffer map
			frame_buffer_map_.erase(current_frame_seen_);

			// Notify main thread that frame is ready
			ready_callback_(current_frame_buffer_id_, current_frame_seen_);

			// Reset current frame seen ID so that if next frame has same number (e.g. repeated
			// sends of single frame 0), it is detected properly
			current_frame_seen_ = -1;
		}
	}

	return frame_state;
}

void PercivalEmulatorFrameDecoder::monitor_buffers(void)
{

    int frames_timedout = 0;
    struct timespec current_time;

    gettime(&current_time);

    // Loop over frame buffers currently in map and check their state
    std::map<uint32_t, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
    while (buffer_map_iter != frame_buffer_map_.end())
    {
        uint32_t frame_num = buffer_map_iter->first;
        int      buffer_id = buffer_map_iter->second;
        void*    buffer_addr = buffer_manager_->get_buffer_address(buffer_id);
        PercivalEmulator::FrameHeader* frame_header = reinterpret_cast<PercivalEmulator::FrameHeader*>(buffer_addr);

        if (elapsed_ms(frame_header->frame_start_time, current_time) > frame_timeout_ms_)
        {
            LOG4CXX_DEBUG_LEVEL(1, logger_, "Frame " << frame_num << " in buffer " << buffer_id
                    << " addr 0x" << std::hex << buffer_addr << std::dec
                    << " timed out with " << frame_header->packets_received << " packets received");

            frame_header->frame_state = FrameReceiveStateTimedout;
            ready_callback_(buffer_id, frame_num);
            frames_timedout++;

            frame_buffer_map_.erase(buffer_map_iter++);
        }
        else
        {
            buffer_map_iter++;
        }
    }
    if (frames_timedout)
    {
        LOG4CXX_WARN(logger_, "Released " << frames_timedout << " timed out incomplete frames");
    }
    frames_timedout_ += frames_timedout;

    LOG4CXX_DEBUG_LEVEL(2, logger_, get_num_mapped_buffers() << " frame buffers in use, "
            << get_num_empty_buffers() << " empty buffers available, "
            << frames_timedout_ << " incomplete frames timed out");

}

#ifdef P2M_EMULATOR_NEW_FIRMWARE
uint16_t PercivalEmulatorFrameDecoder::get_pixel_data_size(void) const
{
    return *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalEmulator::pixel_data_size_offset));
}
#endif

uint8_t PercivalEmulatorFrameDecoder::get_packet_type(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalEmulator::packet_type_offset));
}

uint8_t PercivalEmulatorFrameDecoder::get_subframe_number(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalEmulator::subframe_number_offset));
}

uint32_t PercivalEmulatorFrameDecoder::get_frame_number(void) const
{
    uint32_t frame_number_raw = *(reinterpret_cast<uint32_t*>(raw_packet_header()+PercivalEmulator::frame_number_offset));
    return ntohl(frame_number_raw);
}

uint16_t PercivalEmulatorFrameDecoder::get_packet_number(void) const
{
	uint16_t packet_number_raw = *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalEmulator::packet_number_offset));
    return ntohs(packet_number_raw);
}

#ifdef P2M_EMULATOR_NEW_FIRMWARE
uint16_t PercivalEmulatorFrameDecoder::get_packet_offset(void) const
{
    return *(reinterpret_cast<uint16_t*>(raw_packet_header()+PercivalEmulator::packet_offset_offset));
}
#endif

uint8_t* PercivalEmulatorFrameDecoder::get_frame_info(void) const
{
    return (reinterpret_cast<uint8_t*>(raw_packet_header()+PercivalEmulator::frame_info_offset));
}

uint8_t* PercivalEmulatorFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_packet_header_.get());
}


unsigned int PercivalEmulatorFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}
