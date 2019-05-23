/*
 * ParameterAdjustmentPlugin.cpp
 *
 *  Created on: 6 Aug 2018
 *      Author: Matt Taylor
 */

#include "ParameterAdjustmentPlugin.h"
#include "version.h"

namespace FrameProcessor
{

/**
 * The constructor sets up logging used within the class.
 */
ParameterAdjustmentPlugin::ParameterAdjustmentPlugin() :
    first_frame_number_(DEFAULT_FIRST_FRAME)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.ParameterAdjustmentPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_INFO(logger_, "ParameterAdjustmentPlugin version " << this->get_version_long() << " loaded");

}

/**
 * Destructor.
 */
ParameterAdjustmentPlugin::~ParameterAdjustmentPlugin()
{
  LOG4CXX_TRACE(logger_, "ParameterAdjustmentPlugin destructor.");
}

/**
 * Perform processing on the frame. For the ParameterAdjustmentPlugin class we
 * adjust confiugred parameters by the configured amount, adding the parameter if needed
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void ParameterAdjustmentPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  if (frame->get_frame_number() == first_frame_number_)
  {
    // If at first frame, set the current adjustments to the the configured ones to apply from now on
    LOG4CXX_DEBUG(logger_, "Setting current parameter adjustments to configured values");
    current_parameter_adjustments_ = configured_parameter_adjustments_;
  }

  // Apply any parameter adjustments
  if (current_parameter_adjustments_.size() != 0)
  {
    std::map<std::string, int64_t>::iterator iter;
    for (iter = current_parameter_adjustments_.begin(); iter != current_parameter_adjustments_.end(); ++iter) {
      uint64_t param_value = frame->get_frame_number() + iter->second;
      frame->meta_data().set_parameter<uint64_t>(iter->first, param_value);
    }
  }

  this->push(frame);
}

/**
 * Set configuration options for this Plugin.
 *
 * This sets up the Parameter Adjustment Plugin according to the configuration IpcMessage
 * objects that are received.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void ParameterAdjustmentPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try {
    // Check if we are setting the first frame number
    if (config.has_param(FIRST_FRAME_CONFIG)) {
      first_frame_number_ = (uint64_t) config.get_param<uint64_t>(FIRST_FRAME_CONFIG);
      LOG4CXX_INFO(logger_, "Setting first frame number to " << first_frame_number_);
    }

    // Check if we are setting the parameter adjustment values
    if (config.has_param(PARAMETER_NAME_CONFIG)) {
      OdinData::IpcMessage parameter_config(
              config.get_param<const rapidjson::Value &>(PARAMETER_NAME_CONFIG));
      std::vector <std::string> parameter_names = parameter_config.get_param_names();

      if (parameter_names.size() != 0)
      {
        for (std::vector<std::string>::iterator iter = parameter_names.begin(); iter != parameter_names.end(); ++iter) {
          std::string parameter_name = *iter;
          OdinData::IpcMessage paramConfig(parameter_config.get_param<const rapidjson::Value &>(parameter_name));
          LOG4CXX_INFO(logger_, "Setting adjustment for parameter " << parameter_name << " to " << (int64_t) paramConfig.get_param<int64_t>(PARAMETER_ADJUSTMENT_CONFIG));
          configured_parameter_adjustments_[parameter_name] = (int64_t) paramConfig.get_param<int64_t>(PARAMETER_ADJUSTMENT_CONFIG);
        }
      } else {
        LOG4CXX_INFO(logger_, "Clearing all parameter adjustments");
        configured_parameter_adjustments_.clear();
      }
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
void ParameterAdjustmentPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(get_name() + "/" + FIRST_FRAME_CONFIG, first_frame_number_);

  std::map<std::string, int64_t>::iterator iter;
  for (iter = configured_parameter_adjustments_.begin(); iter != configured_parameter_adjustments_.end(); ++iter) {
    reply.set_param(get_name() + "/" + PARAMETER_NAME_CONFIG + "/" + iter->first + "/" + PARAMETER_ADJUSTMENT_CONFIG, (int64_t)iter->second);
  }
}

int ParameterAdjustmentPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int ParameterAdjustmentPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int ParameterAdjustmentPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string ParameterAdjustmentPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string ParameterAdjustmentPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */
