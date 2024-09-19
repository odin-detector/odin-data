/*
 * DummyTCPFrameDecoder.cpp
 *
 */

#include "DummyTCPFrameDecoder.h"
#include "gettime.h"
#include "version.h"
#include <algorithm>
#include <bitset>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string.h>
#include <unistd.h>

using namespace FrameReceiver;

DummyTCPFrameDecoder::DummyTCPFrameDecoder()
    : FrameDecoderTCP(),
      current_frame_number_(DummyTcpFrameDecoderDefaults::frame_number),
      current_frame_buffer_id_(DummyTcpFrameDecoderDefaults::buffer_id),
      header_size_(DummyTcpFrameDecoderDefaults::header_size),
      frame_size_(DummyTcpFrameDecoderDefaults::max_size),
      num_buffers_(DummyTcpFrameDecoderDefaults::num_buffers),
      frames_dropped_(0), frames_sent_(0), read_so_far_(0),
      receive_state_(FrameDecoder::FrameReceiveStateEmpty) {
  this->logger_ = Logger::getLogger("FR.DummyTCPFrameDecoder");
  LOG4CXX_INFO(logger_, "DummyTCPFrameDecoder version "
                            << this->get_version_long() << " loaded");
  // buffer can fit 5 frames by default
  buffer_size_ = num_buffers_ * frame_size_;
  frame_buffer_.reset(new char[buffer_size_]);
}

//! Destructor for DummyTCPFrameDecoder
DummyTCPFrameDecoder::~DummyTCPFrameDecoder() {}

/**
 * Initialises the frame decoder
 *
 * \param[in] logger The logger
 * \param[in] config_msg The config parameters to initialise with
 */
void DummyTCPFrameDecoder::init(LoggerPtr &logger,
                                OdinData::IpcMessage &config_msg) {}

void DummyTCPFrameDecoder::reset_statistics() {
  FrameDecoderTCP::reset_statistics();
  LOG4CXX_DEBUG_LEVEL(1, logger_, "DummyTCPFrameDecoder resetting statistics");
  current_frame_number_ = DummyTcpFrameDecoderDefaults::frame_number;
  current_frame_buffer_id_ = DummyTcpFrameDecoderDefaults::buffer_id;
  frames_sent_ = 0;
  read_so_far_ = 0;
}

//! Handle a request for the current decoder configuration.
//!
//! This method handles a request for the current decoder configuration, by
//! populating an IPCMessage with all current configuration parameters.
//!
//! \param[in] param_prefix: string prefix to add to each parameter populated in
//! reply \param[in,out] config_reply: IpcMessage to populate with current
//! decoder parameters
//!

void DummyTCPFrameDecoder::request_configuration(
    const std::string param_prefix, OdinData::IpcMessage &config_reply) {
  // Call the base class method to populate parameters
  FrameDecoder::request_configuration(param_prefix, config_reply);
  config_reply.set_param(param_prefix + "frame_size", frame_size_);
}

/**
 * Gets the buffer to use to store the next message, allocating one if necessary
 *
 * \return Pointer to the current buffer
 */
void *DummyTCPFrameDecoder::get_next_message_buffer(void) {
  // only increment buffer when frame is complete
  if (receive_state_ != FrameDecoder::FrameReceiveStateIncomplete) {
    // get id in buffer circularly
    if (current_frame_buffer_id_ + 1 >= num_buffers_)
      current_frame_buffer_id_ = 0;
    else
      current_frame_buffer_id_++;
  }
  current_raw_buffer_ =
      buffer_manager_->get_buffer_address(current_frame_buffer_id_);
  return static_cast<void *>(static_cast<char *>(current_raw_buffer_) +
                             read_so_far_);
}

//! Get the size of the frame buffers required for current operation mode.
//!
//! This method returns the frame buffer size required for the current operation
//! mode.
//!
//! \return size of frame buffer in bytes
//!
const size_t DummyTCPFrameDecoder::get_frame_buffer_size(void) const {
  return buffer_size_;
}

//! Get the size of the frame header.
//!
//! This method returns the size of the frame header used by the decoder.
//!
//! \return size of the frame header in bytes
const size_t DummyTCPFrameDecoder::get_frame_header_size(void) const {
  return header_size_;
}

/**
 * Processes the message in the current buffer
 *
 * \param[in] bytes_received The number of bytes received
 * \return The state after processing
 */
FrameDecoder::FrameReceiveState
DummyTCPFrameDecoder::process_message(size_t bytes_received) {
  if (read_so_far_ + bytes_received == frame_size_) {
    read_so_far_ = 0;
    return FrameDecoder::FrameReceiveStateComplete;
  } else if (read_so_far_ + bytes_received < frame_size_) {
    read_so_far_ += bytes_received;
    // track how many bytes have been received out of the
    // required total and attempt to complete by yielding control back to Rx
    // thread.
    return FrameDecoder::FrameReceiveStateIncomplete;
  } else
    throw "Not Implemented: Can not handle case when too many bytes received";
}

const size_t DummyTCPFrameDecoder::get_next_message_size(void) const {
  return frame_size_ - read_so_far_;
}

void DummyTCPFrameDecoder::monitor_buffers(void) {}

void DummyTCPFrameDecoder::get_status(const std::string param_prefix,
                                      OdinData::IpcMessage &status_msg) {
  status_msg.set_param(param_prefix + "class",
                       std::string("DummyTCPFrameDecoder"));
}

int DummyTCPFrameDecoder::get_version_major() {
  return ODIN_DATA_VERSION_MAJOR;
}

int DummyTCPFrameDecoder::get_version_minor() {
  return ODIN_DATA_VERSION_MINOR;
}

int DummyTCPFrameDecoder::get_version_patch() {
  return ODIN_DATA_VERSION_PATCH;
}

std::string DummyTCPFrameDecoder::get_version_short() {
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string DummyTCPFrameDecoder::get_version_long() {
  return ODIN_DATA_VERSION_STR;
}
