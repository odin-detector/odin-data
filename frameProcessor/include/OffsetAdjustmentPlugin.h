/*
 * OffsetAdjustmentPlugin.h
 *
 *  Created on: 16 Aug 2018
 *      Author: Matt Taylor
 */

#ifndef FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_
#define FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_

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

const int DEFAULT_OFFSET_ADJUSTMENT = 0;
static const std::string OFFSET_ADJUSTMENT_CONFIG = "offset_adjustment";

/**
 * This plugin class alters the frame offset by a configured amount
 *
 */
class OffsetAdjustmentPlugin : public FrameProcessorPlugin
{
public:
  OffsetAdjustmentPlugin();
  virtual ~OffsetAdjustmentPlugin();
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
  /** Offset adjustment to use **/
  int64_t offset_adjustment_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_ */
