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
ParameterAdjustmentPlugin::ParameterAdjustmentPlugin()
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
  // Apply any parameter adjustments
  if (parameter_adjustments_.size() != 0)
  {
    std::map<std::string, int64_t>::iterator iter;
    for (iter = parameter_adjustments_.begin(); iter != parameter_adjustments_.end(); ++iter) {
      try {
        // If no input parameter specified, add to the frame id, otherwise add to the the input parameter
        if (parameter_inputs_.find(iter->first) == parameter_inputs_.end() || parameter_inputs_[iter->first] == "")
        {
          uint64_t param_value = frame->get_frame_number() + iter->second;
          frame->meta_data().set_parameter<uint64_t>(iter->first, param_value);
        }
        else
        {
          std::string input_parameter = parameter_inputs_[iter->first];
          if (frame->meta_data().has_parameter(input_parameter))
          {
            uint64_t param_value = frame->meta_data().get_parameter<uint64_t>(input_parameter) + iter->second;
            frame->meta_data().set_parameter<uint64_t>(iter->first, param_value);
          }
          else
          {
            std::stringstream ss;
            ss << "Unable to get parameter " << input_parameter << " to use as input parameter for adjustment";
            this->set_error(ss.str());
            LOG4CXX_WARN(logger_, ss.str());
          }
        }
      }
      catch (std::exception &e) {
        std::stringstream ss;
        ss << "Error setting parameter adjustment for " << iter->first;
        this->set_error(ss.str());
        LOG4CXX_WARN(logger_, ss.str());
      }

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
    // Check if we are setting the parameter adjustment values
    if (config.has_param(PARAMETER_NAME_CONFIG)) {
      OdinData::IpcMessage parameter_config(
              config.get_param<const rapidjson::Value &>(PARAMETER_NAME_CONFIG));
      std::vector <std::string> parameter_names = parameter_config.get_param_names();

      if (parameter_names.size() != 0)
      {
        for (std::vector<std::string>::iterator iter = parameter_names.begin();
             iter != parameter_names.end(); ++iter) {
          std::string parameter_name = *iter;
          OdinData::IpcMessage paramConfig(parameter_config.get_param<const rapidjson::Value &>(parameter_name));

          LOG4CXX_INFO(logger_, "Setting adjustment for parameter " << parameter_name << " to "
              << (int64_t) paramConfig.get_param<int64_t>(PARAMETER_ADJUSTMENT_CONFIG));
          parameter_adjustments_[parameter_name] = (int64_t) paramConfig.get_param<int64_t>(PARAMETER_ADJUSTMENT_CONFIG);

          if (paramConfig.has_param(PARAMETER_INPUT_CONFIG))
          {
            LOG4CXX_INFO(logger_, "Setting input for parameter " << parameter_name << " to "
                << paramConfig.get_param<std::string>(PARAMETER_INPUT_CONFIG));
            parameter_inputs_[parameter_name] = paramConfig.get_param<std::string>(PARAMETER_INPUT_CONFIG);
          }
          else
          {
            parameter_inputs_[parameter_name] = "";
          }
        }
      } else {
        LOG4CXX_INFO(logger_, "Clearing all parameter adjustments");
        parameter_adjustments_.clear();
        parameter_inputs_.clear();
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
  std::map<std::string, int64_t>::iterator iter;
  for (iter = parameter_adjustments_.begin(); iter != parameter_adjustments_.end(); ++iter) {
    reply.set_param(get_name() + "/" + PARAMETER_NAME_CONFIG + "/" + iter->first + "/"
        + PARAMETER_ADJUSTMENT_CONFIG, (int64_t)iter->second);
  }

  std::map<std::string, std::string>::iterator input_iter;
  for (input_iter = parameter_inputs_.begin(); input_iter != parameter_inputs_.end(); ++input_iter) {
    reply.set_param(get_name() + "/" + PARAMETER_NAME_CONFIG + "/" + input_iter->first
        + "/" + PARAMETER_INPUT_CONFIG, input_iter->second);
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
