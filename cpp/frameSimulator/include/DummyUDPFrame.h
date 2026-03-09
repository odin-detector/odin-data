#ifndef FRAMESIMULATOR_DUMMYUDPFRAME_H
#define FRAMESIMULATOR_DUMMYUDPFRAME_H

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include "Packet.h"

namespace FrameSimulator {

struct DummyUDPPacketHeader {
    uint32_t frame_number;
    uint32_t packet_number_flags;
};

}

#endif // FRAMESIMULATOR_DUMMYUDPFRAME_H
