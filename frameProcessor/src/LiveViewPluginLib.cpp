/*
 * LiveViewPluginLib.cpp
 *
 *  Created on: 11 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#include "LiveViewPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, LiveViewPlugin, "LiveViewPlugin");

} // namespace FrameProcessor



