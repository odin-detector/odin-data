/*
 * OffsetAdjustmentPlugin.h
 *
 *  Created on: 16 Aug 2018
 *      Author: vtu42223
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
const int DEFAULT_OFFSET_FIRST_FRAME = 0;
static const std::string OFFSET_ADJUSTMENT_CONFIG = "offset_adjustment";
static const std::string FIRST_FRAME_OFFSET_CONFIG = "first_frame_number";

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

private:
  void requestConfiguration(OdinData::IpcMessage& reply);

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Offset adjustment currently in use **/
  int64_t current_offset_adjustment_;
  /** Offset adjustment configured to be used from next first frame onwards **/
  int64_t configured_offset_adjustment_;
  /** Frame number of the first frame in an acquisition from the detector **/
  uint64_t first_frame_number_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_ */
