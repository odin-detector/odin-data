#ifndef FRAMESIMULATOR_DUMMYUDPFRAME_H
#define FRAMESIMULATOR_DUMMYUDPFRAME_H

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <stdint.h>

#include "Packet.h"

namespace FrameSimulator {

    class DummyUDPFrameSimulatorPlugin;

    typedef std::vector<Packet> PacketList;

    /** DummyUDPFrame class
     *
     * A dummy UDP frame and its packets
     */

    class DummyUDPFrame {

        friend class DummyUDPFrameSimulatorPlugin;

    public:

        DummyUDPFrame(const int& frameNum);

    private:

        PacketList packets;

        int frame_number;
        std::vector<int> SOF_markers;
        std::vector<int> EOF_markers;

    };

    struct DummyUDPPacketHeader {
        uint32_t frame_number;
        uint32_t packet_number_flags;
    };
}

#endif //FRAMESIMULATOR_DUMMYUDPFRAME_H
