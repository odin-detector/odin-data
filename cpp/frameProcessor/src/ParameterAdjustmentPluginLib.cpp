/*
 * ParameterAdjustmentPluginLib.cpp
 *
 *  Created on: 10 Aug 2018
 *      Author: Matt Taylor
 */

#include "ParameterAdjustmentPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, ParameterAdjustmentPlugin, "ParameterAdjustmentPlugin");

} // namespace FrameReceiver


