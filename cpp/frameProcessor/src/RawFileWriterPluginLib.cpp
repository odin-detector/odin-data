/*
 * RawFileWriterPlugin.cpp
 *
 *  Created on: 9 Feb 2026
 *      Author: Famous Alele
 */

#include "RawFileWriterPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
	REGISTER(FrameProcessorPlugin, RawFileWriterPlugin, "RawFileWriterPlugin");

} // namespace FrameProcessor
