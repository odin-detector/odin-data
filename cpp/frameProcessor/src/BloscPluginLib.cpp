/*
 * BloscPluginLib.cpp
 *
 */

#include "BloscPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
	REGISTER(FrameProcessorPlugin, BloscPlugin, "BloscPlugin");

} // namespace FrameProcessor



