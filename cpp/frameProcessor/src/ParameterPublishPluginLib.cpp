#include "ParameterPublishPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader. This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, ParameterPublishPlugin, "ParameterPublishPlugin");

}  // namespace FrameProcessor
