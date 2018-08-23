/*
 * OffsetAdjustmentPluginLib.cpp
 *
 *  Created on: 16 Aug 2018
 *      Author: vtu42223
 */

#include "OffsetAdjustmentPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, OffsetAdjustmentPlugin, "OffsetAdjustmentPlugin");

} // namespace FrameReceiver



