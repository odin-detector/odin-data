/*
 * ParameterAdjustmentPlugin.h
 *
 *  Created on: 6 Aug 2018
 *      Author: Matt Taylor
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
const int DEFAULT_FIRST_FRAME = 0;
static const std::string PARAMETER_NAME_CONFIG = "parameter";
static const std::string PARAMETER_ADJUSTMENT_CONFIG = "adjustment";
static const std::string FIRST_FRAME_CONFIG = "first_frame_number";

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
  /** Parameter adjustments currently in use **/
  std::map<std::string, int64_t> current_parameter_adjustments_;
  /** Parameter adjustments configured to be used from next first frame onwards **/
  std::map<std::string, int64_t> configured_parameter_adjustments_;
  /** Frame number of the first frame in an acquisition from the detector **/
  uint64_t first_frame_number_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_ */
