/*
 * KafkaProducerPluginLib.cpp
 *
 *  Created on: 25 Mar 2019
 *      Author: Emilio Perez
 */
#include "KafkaProducerPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
  /**
   * Registration of this plugin through the ClassLoader. This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameProcessorPlugin, KafkaProducerPlugin, "KafkaProducerPlugin");

}  // namespace FrameProcessor
