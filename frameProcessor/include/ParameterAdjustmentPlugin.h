/*
 * ParameterAdjustmentPlugin.h
 *
 *  Created on: 6 Aug 2018
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_
#define FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
static const std::string PARAMETER_NAME_CONFIG = "parameter";
static const std::string PARAMETER_INPUT_CONFIG = "input";
static const std::string PARAMETER_ADJUSTMENT_CONFIG = "adjustment";

/**
 * This plugin class alters parameters named in a list by a configured amount added on
 * to the frame number. The parameter will be added to the frame if it doesn't already exist
 *
 */
class ParameterAdjustmentPlugin : public FrameProcessorPlugin
{
public:
  ParameterAdjustmentPlugin();
  virtual ~ParameterAdjustmentPlugin();
  void process_frame(boost::shared_ptr<Frame> frame);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

private:
  void requestConfiguration(OdinData::IpcMessage& reply);

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Map of parameter adjustments to use for each parameter **/
  std::map<std::string, int64_t> parameter_adjustments_;
  /** Map of input parameters to use for each parameter **/
  std::map<std::string, std::string> parameter_inputs_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_ */
