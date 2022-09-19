/*
 * DummyUDPFrameSimulatorPluginLib.cpp
 *
 *  Created on: 19 Sep 2022
 *      Author: Gary Yendell
 */

#include "DummyUDPFrameSimulatorPlugin.h"
#include "ClassLoader.h"

namespace FrameSimulator
{
  /**
   * Registration of this decoder through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameSimulatorPlugin, DummyUDPFrameSimulatorPlugin, "DummyUDPFrameSimulatorPlugin");

}
