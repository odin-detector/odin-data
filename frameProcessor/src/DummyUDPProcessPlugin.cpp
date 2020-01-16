/*
 * DummyUDPProcessPlugin.cpp
 * 
 *  Created on: 22 Jul 2019
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include "DummyUDPProcessPlugin.h"
#include "version.h"

namespace FrameProcessor
{

  // Configuration parameter name definitions
  const std::string DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH = "width";
  const std::string DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT = "height";
  const std::string DummyUDPProcessPlugin::CONFIG_COPY_FRAME = "copy_frame";

  /**
   * The constructor sets up default configuration parameters and logging used within the class.
   */
  DummyUDPProcessPlugin::DummyUDPProcessPlugin() :
    image_width_(1400),
    image_height_(1024),
    packets_lost_(0),
    copy_frame_(true)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.DummyUDPProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_INFO(logger_, "DummyUDPProcessPlugin version " << this->get_version_long() << " loaded");
  }

  /**
   * Destructor.
   */
  DummyUDPProcessPlugin::~DummyUDPProcessPlugin()
  {
    LOG4CXX_TRACE(logger_, "DummyUDPProcessPlugin destructor.");
  }

  /**
   * Get the plugin major version number.
   * 
   * \return major version number as an integer
   */ 
  int DummyUDPProcessPlugin::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  /**
   * Get the plugin minor version number.
   * 
   * \return minor version number as an integer
   */ 
  int DummyUDPProcessPlugin::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  /**
   * Get the plugin patch version number.
   * 
   * \return patch version number as an integer
   */ 
  int DummyUDPProcessPlugin::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

 /**
   * Get the plugin short version (e.g. x.y.z) string.
   * 
   * \return short version as a string
   */ 
  std::string DummyUDPProcessPlugin::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  /**
   * Get the plugin long version (e.g. x.y.z-qualifier) string.
   * 
   * \return long version as a string
   */ 
  std::string DummyUDPProcessPlugin::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

  /**
   * Configure the plugin.  This receives an IpcMessage which should be processed
   * to configure the plugin, and any response can be added to the reply IpcMessage.
   *
   * \param[in] config - Reference to the configuration IpcMessage object.
   * \param[out] reply - Reference to the reply IpcMessage object.
   */
  void DummyUDPProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {

    if (config.has_param(DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH))
    {
      image_width_ = config.get_param<int>(DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH);
      LOG4CXX_DEBUG(logger_, "Setting image width to " << image_width_);
    }

    if (config.has_param(DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT))
    {
      image_height_ = config.get_param<int>(DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT);
      LOG4CXX_DEBUG(logger_, "Setting image height to " << image_height_);
    }

    if (config.has_param(DummyUDPProcessPlugin::CONFIG_COPY_FRAME))
    {
      copy_frame_ = config.get_param<bool>(DummyUDPProcessPlugin::CONFIG_COPY_FRAME);
      LOG4CXX_DEBUG(logger_, "Setting copy frame mode to " << (copy_frame_ ? "true" : "false"));
    }

  }

  /**
   * Respond to configuration requests from clients.
   * 
   * This method responds to configuration requests from client, populating the supplied IpcMessage
   * reply with configured parameters.
   * 
   * \param[in,out] reply - IpcMessage response to client, to be populated with plugin parameters
   */
  void DummyUDPProcessPlugin::requestConfiguration(OdinData::IpcMessage& reply)
  {

    std::string base_str = get_name() + "/";
    reply.set_param(base_str + DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH, image_width_);
    reply.set_param(base_str + DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT, image_height_);
    reply.set_param(base_str + DummyUDPProcessPlugin::CONFIG_COPY_FRAME, copy_frame_);
  }

  /**
   * Collate status information for the plugin.  The status is added to the status IpcMessage object.
   *
   * \param[out] status - Reference to an IpcMessage value to store the status.
   */
  void DummyUDPProcessPlugin::status(OdinData::IpcMessage& status)
  {
    std::string base_str = get_name() + "/";
    status.set_param(base_str + "packets_lost", packets_lost_);
  }

  /**
   * Reset process plugin statistics, i.e. counter of packets lost
   */
  bool DummyUDPProcessPlugin::reset_statistics(void)
  {
    packets_lost_ = 0;
    
    return true;
  }

  /**
   * Perform processing on the frame. For the DummyUDPProcessPlugin class this involves dealing
   * with any lost packets and then simply copying the data buffer into an appropriately 
   * dimensioned output frame.
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void DummyUDPProcessPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Received a new frame");

    DummyUDP::FrameHeader* hdr_ptr = static_cast<DummyUDP::FrameHeader*>(frame->get_data_ptr());

    LOG4CXX_DEBUG(logger_, "Raw frame number: " << hdr_ptr->frame_number);
    LOG4CXX_DEBUG(logger_, "Frame state: " << hdr_ptr->frame_state);
    LOG4CXX_DEBUG(logger_, "Packets expected: " << hdr_ptr->total_packets_expected);
    LOG4CXX_DEBUG(logger_, "Packets received: " << hdr_ptr->total_packets_received
      << " expected: " << hdr_ptr->total_packets_expected);
    LOG4CXX_DEBUG(logger_, "Packet size: " << hdr_ptr->packet_size);

    // Process any lost packets
    this->process_lost_packets(frame);

    // Obtain a pointer to the start of the data in the frame
    const void* data_ptr = static_cast<const void*>(
      static_cast<const char*>(frame->get_data_ptr()) + sizeof(DummyUDP::FrameHeader)
    );

    // Create and populate metadata for the output frame
    FrameMetaData frame_meta;

    frame_meta.set_dataset_name("dummy");
    frame_meta.set_data_type(raw_16bit);
    frame_meta.set_frame_number(static_cast<long long>(hdr_ptr->frame_number));
    dimensions_t dims(2);
    dims[0] = image_height_;
    dims[1] = image_width_;
    frame_meta.set_dimensions(dims);
    frame_meta.set_compression_type(no_compression);

    // Calculate output image size
    const std::size_t output_image_size = image_width_ * image_height_ * sizeof(uint16_t);

    // If copy frame mode is selected, create a new data block frame, copy the image data
    // in and push it. Otherwise, set metadata, image size and offset on the current incoming
    // frame, typically a shared buffer frame, and push that.

    if (copy_frame_) 
    {
      LOG4CXX_DEBUG(logger_, "Copying data for frame " << hdr_ptr->frame_number << " to output");

      // Construct a new data block object to output the processed frame 
      boost::shared_ptr<Frame> output_frame;
      output_frame = boost::shared_ptr<Frame>(new DataBlockFrame(frame_meta, output_image_size));

      // Get a pointer to the data buffer in the output frame
      void* output_ptr = output_frame->get_data_ptr();

      // Copy processed frame data into the output frame
      memcpy(output_ptr, data_ptr, output_image_size);

      // Push the output frame 
      this->push(output_frame);
    }
    else
    {
      LOG4CXX_DEBUG(logger_, "Using existing frame " << hdr_ptr->frame_number << " for output");

      // Set metadata on existing frame
      frame->set_meta_data(frame_meta);

      // Set iamge offset on exisitng frame
      frame->set_image_offset(sizeof(DummyUDP::FrameHeader));

      // Set output image size on existing frame
      frame->set_image_size(output_image_size);

      // Push the existing frame
      this->push(frame);
    }
  }

  /**
   * Process and report lost UDP packets for the frame
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void DummyUDPProcessPlugin::process_lost_packets(boost::shared_ptr<Frame>& frame)
  {
    const DummyUDP::FrameHeader* hdr_ptr = 
      static_cast<const DummyUDP::FrameHeader*>(frame->get_data_ptr());

    // Process lost packets if frame header reports any missing
    int hdr_packets_lost = (hdr_ptr->total_packets_expected - hdr_ptr->total_packets_received);
    if (hdr_packets_lost > 0)
    {

      LOG4CXX_DEBUG(logger_,  "Processing " << hdr_packets_lost 
        << " lost packets for frame " << hdr_ptr->frame_number);

      int packets_lost = 0;
      char* payload_ptr = static_cast<char*>(frame->get_data_ptr()) + sizeof(DummyUDP::FrameHeader);

      // Loop over all packets in frame
      for (int packet_idx = 0; packet_idx <  hdr_ptr->total_packets_expected; packet_idx++)
      {
        // If header reports packet missing, zero out the packet
        if (hdr_ptr->packet_state[packet_idx] == 0)
        {
          LOG4CXX_DEBUG(logger_, "Missing packet number " << packet_idx);
          char* packet_ptr = payload_ptr + (hdr_ptr->packet_size * packet_idx);
          memset(packet_ptr, 0, hdr_ptr->packet_size);
          packets_lost++;
        }
      }
      // Check if there's a mismatch between packets reported lost by the header and found
      // by scanning the packet state information.
      if (packets_lost != hdr_packets_lost)
      {
        LOG4CXX_WARN(logger_, "Packet loss mismatch between frame header ("
          << hdr_packets_lost << ") and packet state (" << packets_lost << ")");
      }
      packets_lost_ += packets_lost;
    }
    else
    {
      LOG4CXX_DEBUG(logger_, "No lost packets to process on frame " << hdr_ptr->frame_number);
    }
  }

  
} /* namespace FrameProcessor */
