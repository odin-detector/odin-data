//
// Created by hir12111 on 25/03/19.
//

#include "KafkaProducerPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader. This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, KafkaProducer, "KafkaProducerPlugin");

}  // namespace FrameProcessor
