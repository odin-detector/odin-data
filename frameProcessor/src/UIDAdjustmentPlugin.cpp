/*
 * UIDAdjustmentPlugin.cpp
 *
 *  Created on: 6 Aug 2018
 *      Author: vtu42223
 */

#include "UIDAdjustmentPlugin.h"

namespace FrameProcessor
{

/**
 * The constructor sets up logging used within the class.
 */
UIDAdjustmentPlugin::UIDAdjustmentPlugin() :
    current_uid_adjustment_(DEFAULT_UID_ADJUSTMENT),
    configured_uid_adjustment_(DEFAULT_UID_ADJUSTMENT),
    first_frame_number_(DEFAULT_FIRST_FRAME)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FW.UIDAdjustmentPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "UIDAdjustmentPlugin constructor.");
}

/**
 * Destructor.
 */
UIDAdjustmentPlugin::~UIDAdjustmentPlugin()
{
  LOG4CXX_TRACE(logger_, "UIDAdjustmentPlugin destructor.");
}

/**
 * Perform processing on the frame. For the UIDAdjustmentPlugin class we
 * adjust a parameter called UID (if it exists) by the configured amount
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void UIDAdjustmentPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  if (frame->has_parameter(UID_PARAM_NAME))
  {
    if (frame->get_frame_number() == first_frame_number_)
    {
      // If at first frame, set the offset value to the the configured value to apply it from now on
      current_uid_adjustment_ = configured_uid_adjustment_;
    }

    uint64_t uid_adjusted = frame->get_i64_parameter(UID_PARAM_NAME) + current_uid_adjustment_;
    frame->set_parameter(UID_PARAM_NAME, uid_adjusted);
  }
  this->push(frame);
}

/**
 * Set configuration options for this Plugin.
 *
 * This sets up the UID Adjustment Plugin according to the configuration IpcMessage
 * objects that are received.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void UIDAdjustmentPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try {
    // Check if we are setting the uid adjustment
    if (config.has_param(UID_ADJUSTMENT_CONFIG)) {
      configured_uid_adjustment_ = (uint64_t) config.get_param<uint64_t>(UID_ADJUSTMENT_CONFIG);
      LOG4CXX_INFO(logger_, "Setting uid adjustment to " << configured_uid_adjustment_);
    }

    // Check if we are setting the first frame number
    if (config.has_param(FIRST_FRAME_CONFIG)) {
      first_frame_number_ = (uint64_t) config.get_param<uint64_t>(FIRST_FRAME_CONFIG);
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

} /* namespace FrameProcessor */
