/*
 * GapFillPluginLib.cpp
 *
 *  Created on: 5 Feb 2019
 *      Author: gnx91527
 */
#include "GapFillPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, GapFillPlugin, "GapFillPlugin");

} // namespace FrameProcessor
