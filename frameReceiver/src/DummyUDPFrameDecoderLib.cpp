/*
 * DummyUDPFrameDecoderLib.cpp
 *
 *  Created on: Jun 20, 2017
 *      Author: tcn45
 */

#include "DummyUDPFrameDecoder.h"
#include "ClassLoader.h"

namespace FrameReceiver
{
  /**
   * Registration of this decoder through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameDecoder, DummyUDPFrameDecoder, "DummyUDPFrameDecoder");

}
// namespace FrameReceiver




