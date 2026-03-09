/*
 * DummyUDPFrameDecoderLib.cpp
 *
 *  Created on: 22 Jul 2019
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include "ClassLoader.h"
#include "DummyUDPFrameDecoder.h"

namespace FrameReceiver {
/**
 * Registration of this decoder through the ClassLoader.  This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameDecoder, DummyUDPFrameDecoder, "DummyUDPFrameDecoder");

}
// namespace FrameReceiver
