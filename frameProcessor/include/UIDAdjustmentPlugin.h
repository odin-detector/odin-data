/*
 * UIDAdjustmentPlugin.h
 *
 *  Created on: 6 Aug 2018
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_UIDADJUSTMENTPLUGIN_H_
#define FRAMEPROCESSOR_UIDADJUSTMENTPLUGIN_H_

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

const int DEFAULT_UID_ADJUSTMENT = 0;
const int DEFAULT_FIRST_FRAME = 0;
static const std::string UID_ADJUSTMENT_CONFIG = "uid_adjustment";
static const std::string FIRST_FRAME_CONFIG = "first_frame_number";
static const std::string UID_PARAM_NAME = "UID";

/**
 * This plugin class alters a parameter named UID if one exists by a configured amount
 *
 */
class UIDAdjustmentPlugin : public FrameProcessorPlugin
{
public:
  UIDAdjustmentPlugin();
  virtual ~UIDAdjustmentPlugin();
  void process_frame(boost::shared_ptr<IFrame> frame);
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
  /** UID adjustment currently in use **/
  uint64_t current_uid_adjustment_;
  /** UID adjustment configured to be used from next first frame onwards **/
  uint64_t configured_uid_adjustment_;
  /** Frame number of the first frame in an acquisition from the detector **/
  uint64_t first_frame_number_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_UIDADJUSTMENTPLUGIN_H_ */
