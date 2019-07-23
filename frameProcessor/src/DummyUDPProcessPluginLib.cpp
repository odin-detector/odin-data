/*
 * DummyUDPProcessPluginLib.cpp
 *
 *  Created on: 22 July 2017
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include "DummyUDPProcessPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
	REGISTER(FrameProcessorPlugin, DummyUDPProcessPlugin, "DummyUDPProcessPlugin");

} // namespace FrameReceiver



