/*
 * OffsetAdjustmentPlugin.cpp
 *
 *  Created on: 16 Aug 2018
 *      Author: vtu42223
 */

#include "OffsetAdjustmentPlugin.h"
#include "version.h"

namespace FrameProcessor
{

/**
 * The constructor sets up logging used within the class.
 */
OffsetAdjustmentPlugin::OffsetAdjustmentPlugin() :
    current_offset_adjustment_(DEFAULT_OFFSET_ADJUSTMENT),
    configured_offset_adjustment_(DEFAULT_OFFSET_ADJUSTMENT),
    first_frame_number_(DEFAULT_OFFSET_FIRST_FRAME)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.OffsetAdjustmentPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_INFO(logger_, "OffsetAdjustmentPlugin version " << this->get_version_long() << " loaded");

}

/**
 * Destructor.
 */
OffsetAdjustmentPlugin::~OffsetAdjustmentPlugin()
{
  LOG4CXX_TRACE(logger_, "OffsetAdjustmentPlugin destructor.");
}

/**
 * Perform processing on the frame. For the OffsetAdjustmentPlugin class we
 * adjust the frame offset by the configured amount
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void OffsetAdjustmentPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  if (frame->get_frame_number() == first_frame_number_)
  {
    // If at first frame, set the offset value to the the configured value to apply it from now on
    current_offset_adjustment_ = configured_offset_adjustment_;
  }

  frame->meta_data().adjust_frame_offset(current_offset_adjustment_);
  this->push(frame);
}

/**
 * Set configuration options for this Plugin.
 *
 * This sets up the Offset Adjustment Plugin according to the configuration IpcMessage
 * objects that are received.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void OffsetAdjustmentPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try {
    // Check if we are setting the uid adjustment
    if (config.has_param(OFFSET_ADJUSTMENT_CONFIG)) {
      configured_offset_adjustment_ = (int64_t) config.get_param<int64_t>(OFFSET_ADJUSTMENT_CONFIG);
      LOG4CXX_INFO(logger_, "Setting offset adjustment to " << configured_offset_adjustment_);
    }

    // Check if we are setting the first frame number
    if (config.has_param(FIRST_FRAME_OFFSET_CONFIG)) {
      first_frame_number_ = (uint64_t) config.get_param<uint64_t>(FIRST_FRAME_OFFSET_CONFIG);
      LOG4CXX_INFO(logger_, "Setting first frame number to " << first_frame_number_);
    }
  }
  catch (std::runtime_error& e)
  {
    std::stringstream ss;
    ss << "Bad ctrl msg: " << e.what();
    this->set_error(ss.str());
    throw;
  }
}

/**
 * Get the configuration values for this Plugin.
 *
 * \param[out] reply - Response IpcMessage.
 */
void OffsetAdjustmentPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(get_name() + "/" + FIRST_FRAME_OFFSET_CONFIG, first_frame_number_);
  reply.set_param(get_name() + "/" + OFFSET_ADJUSTMENT_CONFIG, configured_offset_adjustment_);
}

int OffsetAdjustmentPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int OffsetAdjustmentPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int OffsetAdjustmentPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string OffsetAdjustmentPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string OffsetAdjustmentPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */
