#ifndef FRAMESIMULATOR_UDPFRAME_H
#define FRAMESIMULATOR_UDPFRAME_H

#include "Packet.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace FrameSimulator {

    typedef std::vector<boost::shared_ptr<Packet> > PacketList;

    /** UDPFrame class
     *
     * An UDP frame and its packets; defines the SOF and EOF markers
     * for reading a UDP frame from a pcap file
     */

    struct UDPFrame {

        UDPFrame(const int& frameNum);

        PacketList packets;

        int frame_number;
        int trailer_frame_num;
        std::vector<int> SOF_markers;
        std::vector<int> EOF_markers;

    };

}

#endif //FRAMESIMULATOR_UDPFRAME_H
