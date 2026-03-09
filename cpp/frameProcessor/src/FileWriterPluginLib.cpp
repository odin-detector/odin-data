/*
 * FileWriterPluginLib.cpp
 *
 *  Created on: 8 Aug 2017
 *      Author: gnx91527
 */

#include "ClassLoader.h"
#include "FileWriterPlugin.h"

namespace FrameProcessor {
/**
 * Registration of this plugin through the ClassLoader.  This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, FileWriterPlugin, "FileWriterPlugin");

} // namespace FrameReceiver
