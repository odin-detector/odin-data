//
// Created by hir12111 on 30/10/18.
//

#include "SumPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader. This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, SumPlugin, "SumPlugin");

}  // namespace FrameProcessor
