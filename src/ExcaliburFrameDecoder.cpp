/*
 * ExcaliburFrameDecoder.cpp
 *
 *  Created on: Jan 16th, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "ExcaliburFrameDecoder.h"
#include "gettime.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h>

using namespace FrameReceiver;

ExcaliburFrameDecoder::ExcaliburFrameDecoder(LoggerPtr& logger,
        bool enable_packet_logging, unsigned int frame_timeout_ms) :
        FrameDecoder(logger, enable_packet_logging),
		current_frame_seen_(-1),
		current_frame_buffer_id_(-1),
		current_frame_buffer_(0),
		current_frame_header_(0),
		dropping_frame_data_(false),
		frame_timeout_ms_(frame_timeout_ms),
		frames_timedout_(0)
{
    current_packet_header_.reset(new uint8_t[sizeof(Excalibur::PacketHeader)]);
    dropped_frame_buffer_.reset(new uint8_t[Excalibur::total_frame_size]);

    if (enable_packet_logging_) {
        LOG4CXX_INFO(packet_logger_, "PktHdr: SourceAddress");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               SourcePort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     DestinationPort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      SubframeCounter  [4 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |           PacketCounter&Flags [4 Bytes]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |           |");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |-------------- |---- |----  |---------- |----------");
    }
}

ExcaliburFrameDecoder::~ExcaliburFrameDecoder()
{
}

const size_t ExcaliburFrameDecoder::get_frame_buffer_size(void) const
{
    return Excalibur::total_frame_size;
}

const size_t ExcaliburFrameDecoder::get_frame_header_size(void) const
{
    return sizeof(Excalibur::FrameHeader);
}

const size_t ExcaliburFrameDecoder::get_packet_header_size(void) const
{
    return sizeof(Excalibur::PacketHeader);
}

void* ExcaliburFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header_.get();
}

void ExcaliburFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    // Dump raw header if packet logging enabled
    if (enable_packet_logging_)
    {
        std::stringstream ss;
        uint8_t* hdr_ptr = reinterpret_cast<uint8_t*>(current_packet_header_.get());
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex;
        for (unsigned int hdr_byte = 0; hdr_byte < sizeof(Excalibur::PacketHeader); hdr_byte++)
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

	uint32_t subframe_counter = get_subframe_counter();
	uint32_t packet_number = get_packet_number();
	bool start_of_frame_marker = get_start_of_frame_marker();
	bool end_of_frame_marker = get_end_of_frame_marker();

	uint32_t subframe_idx = subframe_counter % 2;
	uint32_t frame = subframe_counter / 2;

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Got packet header:"
            << " packet: "   << packet_number << " subframe ctr: "<< subframe_counter << " idx:" << subframe_idx
			<< " SOF: " << (int)start_of_frame_marker << " EOF: " << (int)end_of_frame_marker
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
            current_frame_header_ = reinterpret_cast<Excalibur::FrameHeader*>(current_frame_buffer_);
            current_frame_header_->frame_number = current_frame_seen_;
            current_frame_header_->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
            current_frame_header_->packets_received = 0;
            current_frame_header_->sof_marker_count = 0;
            current_frame_header_->eof_marker_count = 0;
            memset(current_frame_header_->packet_state, 0, Excalibur::num_frame_packets);
            gettime(reinterpret_cast<struct timespec*>(&(current_frame_header_->frame_start_time)));

    	}
    	else
    	{
    		current_frame_buffer_id_ = frame_buffer_map_[current_frame_seen_];
        	current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
        	current_frame_header_ = reinterpret_cast<Excalibur::FrameHeader*>(current_frame_buffer_);
    	}

    }

    // If SOF or EOF markers seen in packet header, increment appropriate field in frame header
    if (start_of_frame_marker) {
    	(current_frame_header_->sof_marker_count)++;
    }
    if (end_of_frame_marker) {
    	(current_frame_header_->eof_marker_count)++;
    }

    // Update packet_number state map in frame header
    current_frame_header_->packet_state[subframe_idx][packet_number] = 1;

}

void* ExcaliburFrameDecoder::get_next_payload_buffer(void) const
{

    uint8_t* next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_) +
            get_frame_header_size() +
            (Excalibur::subframe_size * (get_subframe_counter() % 2)) +
            (Excalibur::primary_packet_size * get_packet_number());

    return reinterpret_cast<void*>(next_receive_location);
}

size_t ExcaliburFrameDecoder::get_next_payload_size(void) const
{
   size_t next_receive_size = 0;

	if (get_packet_number() < Excalibur::num_primary_packets)
	{
		next_receive_size = Excalibur::primary_packet_size;
	}
	else
	{
		next_receive_size = Excalibur::tail_packet_size;
	}

    return next_receive_size;
}

FrameDecoder::FrameReceiveState ExcaliburFrameDecoder::process_packet(size_t bytes_received)
{

    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

    // If this packet is the last in subframe (i.e. has on EOF marker in the header), extract the frame
    // number (which counts from 1) from the subframe trailer, update and/or validate in the frame buffer
    // header appropriately.
    if (get_end_of_frame_marker()) {

    	size_t payload_bytes_received = bytes_received - sizeof(Excalibur::PacketHeader);

    	Excalibur::SubframeTrailer* trailer = reinterpret_cast<Excalibur::SubframeTrailer*>(
    			(uint8_t*)get_next_payload_buffer() + payload_bytes_received - sizeof(Excalibur::SubframeTrailer));

    	uint32_t frame_number = static_cast<uint32_t>((trailer->frame_number & 0xFFFFFFFF) - 1);
    	LOG4CXX_DEBUG_LEVEL(3, logger_, "Subframe EOF trailer has frame number = " << frame_number);

    	uint32_t subframe_idx = get_subframe_counter() % 2;

    	if (subframe_idx == 0) {
    		current_frame_header_->frame_number = frame_number;
    	} else {
    		if (frame_number != current_frame_header_->frame_number) {
    			LOG4CXX_WARN(logger_, "Subframe EOF trailer frame number mismatch for frame " << current_frame_seen_
    					<< ": got " << frame_number << ", expected " << current_frame_header_->frame_number
    					);
    		}
    	}
    }

	current_frame_header_->packets_received++;

	if (current_frame_header_->packets_received == Excalibur::num_frame_packets)
	{

		// Check that the appropriate number of SOF and EOF markers (one each per subframe) have
		// been seen, otherwise log a warning

		if ((current_frame_header_->sof_marker_count != Excalibur::num_subframes) ||
			(current_frame_header_->eof_marker_count != Excalibur::num_subframes))
		{
			LOG4CXX_WARN(logger_, "Incorrect number of SOF (" << current_frame_header_->sof_marker_count
					<< ") or EOF (" << current_frame_header_->eof_marker_count
					<< "markers seen on completed frame " << current_frame_seen_
					);
		}

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

void ExcaliburFrameDecoder::monitor_buffers(void)
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
        Excalibur::FrameHeader* frame_header = reinterpret_cast<Excalibur::FrameHeader*>(buffer_addr);

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

uint32_t ExcaliburFrameDecoder::get_subframe_counter(void) const
{
    return reinterpret_cast<Excalibur::PacketHeader*>(current_packet_header_.get())->subframe_counter;
}

uint32_t ExcaliburFrameDecoder::get_packet_number(void) const
{
    return reinterpret_cast<Excalibur::PacketHeader*>(current_packet_header_.get())->packet_number_flags & Excalibur::packet_number_mask;
}

bool ExcaliburFrameDecoder::get_start_of_frame_marker(void) const
{
	uint32_t packet_number_flags = reinterpret_cast<Excalibur::PacketHeader*>(current_packet_header_.get())->packet_number_flags;
	return ((packet_number_flags & Excalibur::start_of_frame_mask) != 0);
}

bool ExcaliburFrameDecoder::get_end_of_frame_marker(void) const
{
	uint32_t packet_number_flags = reinterpret_cast<Excalibur::PacketHeader*>(current_packet_header_.get())->packet_number_flags;
	return ((packet_number_flags & Excalibur::end_of_frame_mask) != 0);
}

unsigned int ExcaliburFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}
