/*
 * TmpFSPlugin.cpp
 *
 *  Created on: 9 Feb 2026
 *      Author: Famous Alele
 */

#include "TmpFSPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
	REGISTER(FrameProcessorPlugin, TmpFSPlugin, "TmpFSPlugin");

} // namespace FrameReceiver
