#ifndef FRAMESIMULATOR_FRAMESIMULATOROPTIONSDUMMYUSP_H
#define FRAMESIMULATOR_FRAMESIMULATOROPTIONSDUMMYUSP_H

#include "FrameSimulatorOption.h"
#include <string>

namespace FrameSimulator {

    static const int default_image_width = 1400;
    static const int default_image_height = 1024;
    static const int default_pkt_len = 8000;

    static const FrameSimulatorOption<int> opt_image_width("width", "Simulated image width");
    static const FrameSimulatorOption<int> opt_image_height("height", "Simulated image height");
    static const FrameSimulatorOption<int> opt_packet_len("packet-len", "Length of simulated packets in bytes");

}
#endif // FRAMESIMULATOR_FRAMESIMULATOROPTIONSDUMMYUSP_H
