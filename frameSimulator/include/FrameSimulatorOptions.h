#ifndef FRAMESIMULATOR_FRAMESIMULATOROPTIONS_H
#define FRAMESIMULATOR_FRAMESIMULATOROPTIONS_H

#include "FrameSimulatorOption.h"
#include <string>

namespace FrameSimulator {

    static const FrameSimulatorOption<std::string> opt_ports("ports", "Provide a (comma separated) port list");
    static const FrameSimulatorOption<int> opt_frames("frames", "Number of frames (-1 sends all frames)");
    static const FrameSimulatorOption<int> opt_interval("interval", "Transmission interval between frames");

}

#endif