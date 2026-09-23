//
// Created by sgr21863 on 08/09/2026.
//

#include "ClassLoader.h"
#include "SharedMemoryPlugin.h"

namespace FrameProcessor {
/**
 * Registration of this plugin through the ClassLoader. This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, SharedMemoryPlugin, "SharedMemoryPlugin");

} // namespace FrameProcessor
