/*
 * FrameDecoder.cpp - abstract base class for frameReceiver decoder plugins
 *
 *  Created on: Nov 6, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameDecoder.h"
#include "FrameReceiverDefaults.h"

using namespace FrameReceiver;

//! Constructor for the FrameDecoder class.
//!
//! This method initialises the base class of each frame decoder, storing default values
//! for base configuration parameters and variables.
//!
FrameDecoder::FrameDecoder() :
     logger_(Logger::getLogger("FR.FrameDecoder")),
     enable_packet_logging_(FrameReceiver::Defaults::default_enable_packet_logging),
     frame_timeout_ms_(FrameReceiver::Defaults::default_frame_timeout_ms),
     frames_timedout_(0),
     frames_dropped_(0)
{
};

//! This overload is scheduled for deletion; logger is unused.
void FrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
    init(config_msg);
}

//! Initialise the FrameDecoder class
//!
//! This method initialises the base FrameDecoder class, extracting and storing the appropriate
//! parameters from the configuration message.
//!
//! \param[in] config_msg - IpcMessage containing decoder configuration parameters
//!
void FrameDecoder::init(OdinData::IpcMessage& config_msg)
{
   enable_packet_logging_ = config_msg.get_param<bool>(
       CONFIG_DECODER_ENABLE_PACKET_LOGGING, enable_packet_logging_);
   frame_timeout_ms_ = config_msg.get_param<unsigned int>(
       CONFIG_DECODER_FRAME_TIMEOUT_MS, frame_timeout_ms_);

   // Retrieve the packet logger instance
   packet_logger_ = Logger::getLogger("FR.PacketLogger");
}

//
//! Handle a configuration request from the controlling application.
//!
//! This method handles a configuration request from the controlling application, populating
//! the parameter block of the reply message with decoder parameters using the specified
//! parameter prefix. This method should be overridden by derived decoder classes and then called
//! by the overridden method to populate base class parameters.
//!
//! \param[in] param_prefix - parameter prefix to use on all parameter names
//! \param[in,out] config_reply - reply IpcMessage to populate with parameters
//!
void FrameDecoder::request_configuration(const std::string param_prefix,
    OdinData::IpcMessage& config_reply)
{
    config_reply.set_param(param_prefix + CONFIG_DECODER_ENABLE_PACKET_LOGGING, enable_packet_logging_);
    config_reply.set_param(param_prefix + CONFIG_DECODER_FRAME_TIMEOUT_MS, frame_timeout_ms_);
}

/** Request the decoder's supported commands.
 *
 * In this abstract class the request method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \return - Vector containing supported command strings.
 */
std::vector<std::string> FrameDecoder::request_commands()
{
  // Default returns an empty vector.
  std::vector<std::string> reply;
  return reply;
}

/** Execute a command within the decoder.
 *
 * In this abstract class the command method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[in] command - String containing the command to execute.
 * \param[out] reply - Response IpcMessage.
 */
void FrameDecoder::execute(const std::string& command, OdinData::IpcMessage& reply)
{
  // A command has been submitted and this decoder has no execute implementation defined,
  // throw a runtime error to report this.
  std::stringstream is;
  is << "Submitted command not supported: " << command;
  LOG4CXX_ERROR(logger_, is.str());
  throw std::runtime_error(is.str().c_str());
}

//! Register a buffer manager with the decoder.
//!
//! This method registers a SharedBufferManager instance with the decoder, to be used when
//! receiving, decoding and storing incoming data.
//!
//! \param[in] buffer_manager - pointer to a SharedBufferManager instance
//!
void FrameDecoder::register_buffer_manager(OdinData::SharedBufferManagerPtr buffer_manager)
{
    buffer_manager_ = buffer_manager;
}

//! Register a frame ready callback with the decoder.
//!
//! This method is used to register a frame ready callback function with the decoder, which is
//! called when the decoder determines that a frame is ready to be released to the downstream
//! processing application.
//!
//! \param[in] callback - callback function with the appropriate signature
//!
void FrameDecoder::register_frame_ready_callback(FrameReadyCallback callback)
  {
      ready_callback_ = callback;
  }

//! Push an buffer onto the empty buffer queue.
//!
//! This method is used to add an empty buffer to the tail of the internal empty buffer
//! queue for subsequent use receiving frame data
//!
//! \param[in] buffer_id - SharedBufferManager buffer ID
//!
void FrameDecoder::push_empty_buffer(int buffer_id)
{
    empty_buffer_queue_.push(buffer_id);
}

//! Get the number of empty buffers queued.
//!
//! This method returns the number of buffers currently queued by the decoder in the empty
//! buffer queue
//!
//! \return number of empty buffers queued
//!
const size_t FrameDecoder::get_num_empty_buffers(void) const
{
    return empty_buffer_queue_.size();
}

//! Get the number of mapped buffers currently held.
//!
//! This method returns the number of buffers currently mapped to incoming frames by the
//! decoder, i.e. frames which are being filled but are not yet ready for processing.
//!
//! \return - number of buffers currently mapped for incoming frames
//!
const size_t FrameDecoder::get_num_mapped_buffers(void) const
{
    return frame_buffer_map_.size();
}

//! Get the current frame timeout value.
//!
//! This method returns the frame timeout in milliseconds currently configured in the decoder.
//!
//! \return - current frame timeout in milliseconds
//!
const unsigned int FrameDecoder::get_frame_timeout_ms(void) const
{
    return frame_timeout_ms_;
}

//! Get the number of frames timed out in the decoder
//!
//! This method returns the number of frames that have timed out during reception
//! by the frame decoder. This is typically determined by specialised decoders subclassed
//! from this class.
//!
//! \return - number of frames timed out
//!
const unsigned int FrameDecoder::get_num_frames_timedout(void) const
{
    return frames_timedout_;
}

//! Get the number of frames dropped in the decoder
//!
//! This method returns the number of frames that have been dropped by the frame decoder.
//! This is typically determined by specialised decoders subclassed from this class.
//!
//! \return - number of frames dropped
//!
const unsigned int FrameDecoder::get_num_frames_dropped(void) const
{
    return frames_dropped_;
}

//! Drop all buffers currently held by the decoder.
//!
//! This method forces the decoder to drop all buffers currently held either in the empty
//! buffer queue or currently mapped to incoming frames. It is intended to be used at
//! configuration time where, e.g. the underlying shared buffer manager has been reconfigured
//! and the current buffer references are thus invalid.
//!
void FrameDecoder::drop_all_buffers(void)
{
  if (!empty_buffer_queue_.empty())
  {
    LOG4CXX_INFO(logger_, "Dropping " << empty_buffer_queue_.size() 
        << " buffers from empty buffer queue");
    EmptyBufferQueue new_queue;
    std::swap(empty_buffer_queue_, new_queue);
  }

  if (!frame_buffer_map_.empty())
  {
    LOG4CXX_WARN(logger_, "Dropping " << frame_buffer_map_.size() <<
                 " unreleased buffers from decoder - possible data loss");
    FrameBufferMap new_map;
    std::swap(frame_buffer_map_, new_map);
  }
}

//! Collate version information for the decoder.
//!
//! The version information is added to the status IpcMessage object.
//! 
//! \param[in,out] status - Reference to an IpcMessage value to store the version.
//!
void FrameDecoder::version(const std::string param_prefix, OdinData::IpcMessage& status)
{
  status.set_param(param_prefix + "major", get_version_major());
  status.set_param(param_prefix + "minor", get_version_minor());
  status.set_param(param_prefix + "patch", get_version_patch());
  status.set_param(param_prefix + "short", get_version_short());
  status.set_param(param_prefix + "full", get_version_long());
}

//! Reset frame decoder statistics.
//!
//! This method resets the frame decoder statistics. In this base class this
//! is limited to setting the timed-out frames count to zero. Dervied decoder classes
//! which override this method should ensure the base class version is explicitly called.
//!
void FrameDecoder::reset_statistics(void)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Resetting frame decoder statistics");
    frames_timedout_ = 0;
    frames_dropped_ = 0;
}
