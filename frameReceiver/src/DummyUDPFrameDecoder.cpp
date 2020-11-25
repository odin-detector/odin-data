/*
 * DummyUDPFrameDecoder.cpp
 * 
 * ODIN data frame decoder plugin for dummy UDP frame streams - used in integration testing
 *
 *  Created on: May 12, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <sstream>

#include "DummyUDPFrameDecoder.h"
#include "version.h"
#include "gettime.h"

using namespace FrameReceiver;

//! Constructor for the DummyUDPFrameDecoder.
//!
//! This constructor sets up the decoder, setting up default values for the packet and frame
//! handling logic of the decoder, and allocating buffers for packet headers, dropped frames and
//! ignored packets.
//!
DummyUDPFrameDecoder::DummyUDPFrameDecoder() :
    FrameDecoderUDP(),
    udp_packets_per_frame_(DummyUdpFrameDecoderDefaults::default_udp_packets_per_frame),
    udp_packet_size_(DummyUdpFrameDecoderDefaults::default_udp_packet_size),
    status_get_count_(0),
    current_frame_seen_(DummyUDP::default_frame_number),
    current_frame_buffer_id_(DummyUDP::default_frame_number),
    current_frame_buffer_(0),
    current_frame_header_(0),
    num_active_fems_(0),
    dropping_frame_data_(false),
    packets_received_(0),
    packets_lost_(0),
    packets_dropped_(0)
{

  // Allocate buffers for packet header, dropped frames and scratched packets
  current_packet_header_.reset(new uint8_t[sizeof(DummyUDP::PacketHeader)]);
  dropped_frame_buffer_.reset(new uint8_t[DummyUDP::max_frame_size()]);

  this->logger_ = Logger::getLogger("FR.DummyUDPFrameDecoder");
  LOG4CXX_INFO(logger_, "DummyFrameDecoderUDP version " << this->get_version_long() << " loaded");
}

//! Destructor for DummyDUPFrameDecoder
DummyUDPFrameDecoder::~DummyUDPFrameDecoder()
{
}

int DummyUDPFrameDecoder::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int DummyUDPFrameDecoder::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int DummyUDPFrameDecoder::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string DummyUDPFrameDecoder::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string DummyUDPFrameDecoder::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

//! Initialise the frame decoder.
//!
//! This method initialises the decoder based on a configuration message passed by the
//! application controller. Parameters found in the decoder configuraiton are parsed and stored
//! in the decoder as appropriate.
//!
//! \param[in] logger - pointer to the message logger
//! \param[in] config_msg - configuration message
//!
void DummyUDPFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{

  // Pass the configuration message to the base class decoder
  FrameDecoderUDP::init(logger_, config_msg);

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Got decoder config message: " << config_msg.encode());

  // Determine the number of UDP packets per frame if present in the config message, checking that
  // it doesn't exceed the maximum permitted
  udp_packets_per_frame_ = config_msg.get_param<unsigned int>(
      CONFIG_DECODER_UDP_PACKETS_PER_FRAME, udp_packets_per_frame_);

  if (udp_packets_per_frame_ > DummyUDP::max_packets)
  {
    std::stringstream sstr;
    sstr << "The requested number of UDP packets (" << udp_packets_per_frame_ 
      << ") exceeds the maximum allowed (" << DummyUDP::max_packets << ")"; 
    throw OdinData::OdinDataException(sstr.str());
  }

  // Determine the UDP packet size to use if present in the config message, chacking that it
  // doesn't exceed the maximum permitted
  udp_packet_size_ = static_cast<std::size_t>(config_msg.get_param<unsigned int>(
    CONFIG_DECODER_UDP_PACKET_SIZE, udp_packet_size_
  ));

  if (udp_packet_size_ > DummyUDP::max_packet_size)
  {
    std::stringstream sstr;
    sstr << "The requested UDP packet size (" << udp_packet_size_
      << ") exceeds the maximum allowed (" << DummyUDP::max_packet_size << ")"; 
    throw OdinData::OdinDataException(sstr.str());
  }
  
  LOG4CXX_DEBUG_LEVEL(3, logger_, "DummyUDPFrameDecoder initialised with "
      << udp_packets_per_frame_ << " UDP packets per frame, packet size " << udp_packet_size_);

  // Reset packet counters
  packets_received_ = 0;
  packets_lost_ = 0;
  packets_dropped_ = 0;

}

//! Handle a request for the current decoder configuration.
//!
//! This method handles a request for the current decoder configuration, by populating an
//! IPCMessage with all current configuration parameters.
//! 
//! \param[in] param_prefix: string prefix to add to each parameter populated in reply
//! \param[in,out] config_reply: IpcMessage to populate with current decoder parameters
//!
void DummyUDPFrameDecoder::request_configuration(const std::string param_prefix,
  OdinData::IpcMessage& config_reply)
{
    // Call the base class method to populate parameters
    FrameDecoder::request_configuration(param_prefix, config_reply);

    // Add current configuration parameters to reply
    config_reply.set_param(param_prefix + CONFIG_DECODER_UDP_PACKETS_PER_FRAME, 
      udp_packets_per_frame_);
    config_reply.set_param(param_prefix + CONFIG_DECODER_UDP_PACKET_SIZE, 
      udp_packet_size_);

}

//! Get the size of the frame buffers required for current operation mode.
//!
//! This method returns the frame buffer size required for the current operation mode, which is
//! determined by the configured number of UDP packets.
//!
//! \return size of frame buffer in bytes
//!
const size_t DummyUDPFrameDecoder::get_frame_buffer_size(void) const
{
  size_t frame_buffer_size = get_frame_header_size() +
      (udp_packets_per_frame_ * udp_packet_size_);
  return frame_buffer_size;
}

//! Get the size of the frame header.
//!
//! This method returns the size of the frame header used by the decoder, which in this case is the
//! DummyUDP frame header.
//!
//! \return size of the frame header in bytes
const size_t DummyUDPFrameDecoder::get_frame_header_size(void) const
{
  return sizeof(DummyUDP::FrameHeader);
}

//! Get the size of a packet header.
//!
//! This method returns the size of a UDP packet header for the receiver thread, which in this case
//! is the size of the DummyUDP packet header.
//!
//! \return size of the packet header in bytes.
//!
const size_t DummyUDPFrameDecoder::get_packet_header_size(void) const
{
  return sizeof(DummyUDP::PacketHeader);
}

//! Get a pointer to the packet header buffer.
//!
//! This method returns a pointer to the next packet header buffer. For this decoder, the packet
//! headers are discarded, so the current packet header is always returned.
//!
//! \return pointer to the packet header buffer
//!
void* DummyUDPFrameDecoder::get_packet_header_buffer(void)
{
  return current_packet_header_.get();
}

//! Process an incoming packet header.
//!
//! This method is called to process the header of an incoming packet. The content of that header is
//! used to determine where the route the payload of the packet on the next receive. Header 
//! information is used to determine which frame buffer the current packet should be routed to,
//! and to request a new frame buffer when the first packet of a given frame is recevied. Buffer
//! exhaustion is also handled by directing all packets for the current frame to a scratch buffer.
//!
//! \param[in] bytes_recevied - number of bytes received
//! \param[in] port - UDP port packet header was received on
//! \param[in] from_addr - socket address structure with details of source packet
//!
void DummyUDPFrameDecoder::process_packet_header(size_t bytes_received, int port, 
  struct sockaddr_in* from_addr)
{

  // Extract fields from the packet header
  uint32_t frame_number = get_frame_number();
  uint32_t packet_number = get_packet_number();

  if (frame_number != current_frame_seen_)
  {
    current_frame_seen_ = frame_number;

    if (frame_buffer_map_.count(current_frame_seen_) == 0)
    {
      if (empty_buffer_queue_.empty())
      {
        current_frame_buffer_ = dropped_frame_buffer_.get();

        if (!dropping_frame_data_)
        {
          LOG4CXX_ERROR(logger_, "First packet from frame " << current_frame_seen_
            << " detected but no free buffers available. Dropping packet data for this frame");
          frames_dropped_++;
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
          LOG4CXX_DEBUG_LEVEL(2, logger_, "First packet from frame " << current_frame_seen_
              << " detected, allocating frame buffer ID " << current_frame_buffer_id_);
        }
        else
        {
          dropping_frame_data_ = false;
          LOG4CXX_DEBUG_LEVEL(2, logger_, "Free buffer now available for frame "
              << current_frame_seen_ << ", allocating frame buffer ID "
              << current_frame_buffer_id_);
        }
      }

      // Initialise frame header
      current_frame_header_ = reinterpret_cast<DummyUDP::FrameHeader*>(current_frame_buffer_);
      initialise_frame_header(current_frame_header_);     

    }
    else
    {
      current_frame_buffer_id_ = frame_buffer_map_[current_frame_seen_];
      current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
      current_frame_header_ = reinterpret_cast<DummyUDP::FrameHeader*>(current_frame_buffer_);
    }
    
  }

  // Update packet state in frame header
  current_frame_header_->packet_state[packet_number] = 1;

  // Increment packet counters
  if (dropping_frame_data_) 
  {
    packets_dropped_++;
  }
  else
  {
    packets_received_++;
  }

}

//! Initialise a frame header
//!
//! This method initialises the frame header specified by the pointer argument, setting
//! fields to their default values, clearing packet counters and setting the active FEM
//! fields as appropriate.
//!
//! \param[in] header_ptr - pointer to frame header to initialise.
//!
void DummyUDPFrameDecoder::initialise_frame_header(DummyUDP::FrameHeader* header_ptr)
{
  header_ptr->frame_number = current_frame_seen_;
  header_ptr->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
  header_ptr->total_packets_expected = udp_packets_per_frame_;
  header_ptr->total_packets_received = 0;
  header_ptr->packet_size = udp_packet_size_;

  memset(header_ptr->packet_state, 0, sizeof(uint8_t) * DummyUDP::max_packets);

  gettime(reinterpret_cast<struct timespec*>(&(header_ptr->frame_start_time)));

}

//! Get a pointer to the next payload buffer.
//!
//! This method returns a pointer to the next packet payload buffer within the appropriate frame.
//! The location of this is determined by state information set during the processing of the packet
//! header.
//!
//! \return pointer to the next payload buffer
//!
void* DummyUDPFrameDecoder::get_next_payload_buffer(void) const 
{
  uint8_t* next_receive_location;

  next_receive_location = reinterpret_cast<uint8_t*>(current_frame_buffer_)
    + get_frame_header_size() + (udp_packet_size_ * get_packet_number());

  return reinterpret_cast<void*>(next_receive_location);
}

//! Get the next packet payload size to receive
//!
//! This method returns the payload size to receive for the next incoming packet.
//!
//! \return size of the next packet payload in bytes
//!
size_t DummyUDPFrameDecoder::get_next_payload_size(void) const 
{
  return udp_packet_size_; 
};

//! Process a received packet payload.
//!
//! This method is called once the payload of a packet has been recevied. In this dummy decoder
//! there is no processing of the content of the payload performed, so this method simply updates
//! the state of the current frame buffer header and the decoder accordingly. If all packets of the
//! frame have been received then the frame state is marked as complete and the frame is handed
//! off to downstream processing.
//!
//! \param[in] bytes_received - number of packet payload bytes received
//! \param[in] port - port the packet was received on
//! \param[in] from_addr - socket address structure indicating source address of packet sender
//! \return current frame receive state
//!
FrameDecoder::FrameReceiveState DummyUDPFrameDecoder::process_packet(
  size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
  FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

  // Increment the packet received counter in the frame header
  current_frame_header_->total_packets_received++;

  // If we have recevied the expected number of packets, mark the frame as complete and hand off
  // the frame for downstream processing
  if (current_frame_header_->total_packets_received == udp_packets_per_frame_)
  {
    frame_state = FrameDecoder::FrameReceiveStateComplete;
    current_frame_header_->frame_state = frame_state;

    if (!dropping_frame_data_)
    {
      // Erase frame from buffer map
      frame_buffer_map_.erase(current_frame_seen_);

      // Notify main thread that frame is ready
      ready_callback_(current_frame_buffer_id_, current_frame_header_->frame_number);

      // Reset current frame seen ID so that if next frame has same number (e.g. repeated
      // sends of single frame 0), it is detected properly
      current_frame_seen_ = -1;     
    }
  }
  return frame_state;
}

//! Monitor the state of currently mapped frame buffers.
//!
//! This method, called periodically by a timer in the receive thread reactor, monitors the state
//! of currently mapped frame buffers. In any frame buffers have been mapped for a sufficiently
//! long time that indicates packets have been lost and the frame is incomplete, the frame is
//! flagged as such and notified to the main thread via the ready callback.
//!
void DummyUDPFrameDecoder::monitor_buffers(void)
{
  int frames_timedout = 0;

  struct timespec current_time;
  gettime(&current_time);

  // Loop over frame buffers currently in map and check their state
  std::map<int, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
  while (buffer_map_iter != frame_buffer_map_.end())
  {
    int frame_num = buffer_map_iter->first;
    int buffer_id = buffer_map_iter->second;

    void *buffer_addr = buffer_manager_->get_buffer_address(buffer_id);

    DummyUDP::FrameHeader* frame_header = reinterpret_cast<DummyUDP::FrameHeader*>(buffer_addr);

    // If the time since the frame starting being filled with packets exceeds a timeout, mark
    // the frame as incomplete and call the ready callback.
    if (elapsed_ms(frame_header->frame_start_time, current_time) > frame_timeout_ms_)
    {
      // Calculated packets lost on this frame and add to total
      uint32_t packets_lost = udp_packets_per_frame_ - frame_header->total_packets_received;
      packets_lost_ += packets_lost;

      LOG4CXX_DEBUG_LEVEL(1, logger_, "Frame " << frame_num << " in buffer " << buffer_id
          << " addr 0x" << std::hex
          << buffer_addr << std::dec << " timed out with " << frame_header->total_packets_received
          << " packets received, " << packets_lost << " packets lost");

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

  LOG4CXX_DEBUG_LEVEL(4, logger_,  get_num_mapped_buffers() << " frame buffers in use, "
      << get_num_empty_buffers() << " empty buffers available, "
      << frames_timedout_ << " incomplete frames timed out, "
      << packets_lost_ << " packets lost"
  );

}

//! Get the current status of the frame decoder.
//!
//! This method populates the IpcMessage passed by reference as an argument with decoder-specific
//! status information, e.g. packet loss by source.
//!
//! \param[in] param_prefix - path to be prefixed to each status parameter name
//! \param[in] status_msg - reference to IpcMesssage to be populated with parameters
//!
void DummyUDPFrameDecoder::get_status(const std::string param_prefix,
    OdinData::IpcMessage& status_msg)
{
  status_get_count_++;
  status_msg.set_param(param_prefix + "name", std::string("DummyUDPFrameDecoder"));
  status_msg.set_param(param_prefix + "status_get_count", status_get_count_);
  status_msg.set_param(param_prefix + "packets_received", packets_received_);
  status_msg.set_param(param_prefix + "packets_lost", packets_lost_);
  status_msg.set_param(param_prefix + "packets_dropped", packets_dropped_);
}

//! Reset the decoder statistics.
//!
//! This method resets the frame decoder statistics, including packets received and lost 
//!
void DummyUDPFrameDecoder::reset_statistics()
{
  FrameDecoderUDP::reset_statistics();
  LOG4CXX_DEBUG_LEVEL(1, logger_, "DummyUDPFrameDecoder resetting statistics");
  status_get_count_ = 0;
  packets_received_ = 0;
  packets_lost_ = 0;
  packets_dropped_ = 0;
}

//! Get the current frame number.
//!
//! This method extracts and returns the frame number from the current UDP packet header.
//!
//! \return current frame number
//!
uint32_t DummyUDPFrameDecoder::get_frame_number(void) const
{
  return reinterpret_cast<DummyUDP::PacketHeader*>(current_packet_header_.get())->frame_number;
}

//! Get the current packet number.
//!
//! This method extracts and returns the packet number from the current UDP packet header.
//!
//! \return current packet number
//!
uint32_t DummyUDPFrameDecoder::get_packet_number(void) const
{
  return reinterpret_cast<DummyUDP::PacketHeader*>(
      current_packet_header_.get())->packet_number_flags & DummyUDP::packet_number_mask;
}

//! Calculate and return an elapsed time in milliseconds.
//!
//! This method calculates and returns an elapsed time in milliseconds based on the start and
//! end timespec structs passed as arguments.
//!
//! \param[in] start - start time in timespec struct format
//! \param[in] end - end time in timespec struct format
//! \return elapsed time between start and end in milliseconds
//!
unsigned int DummyUDPFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

  double start_ns = ((double) start.tv_sec * 1000000000) + start.tv_nsec;
  double end_ns = ((double) end.tv_sec * 1000000000) + end.tv_nsec;

  return (unsigned int)((end_ns - start_ns) / 1000000);
}

