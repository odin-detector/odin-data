#ifndef FRAMESIMULATOR_FRAMESIMULATOROPTIONSUDP_H
#define FRAMESIMULATOR_FRAMESIMULATOROPTIONSUDP_H

#include "FrameSimulatorOption.h"
#include <string>

namespace FrameSimulator {

    /** FrameSimulatorPluginUDP simulator options
     * specific options for a pcap frame simulator
     */
    static const FrameSimulatorOption<std::string> opt_destip("dest-ip", "Destination IP address");
    static const FrameSimulatorOption<std::string> opt_pcapfile("pcap-file", "Packet capture file");
    static const FrameSimulatorOption<int> opt_packetgap("packet-gap", "Pause between N packets");
    static const FrameSimulatorOption<float> opt_dropfrac("drop-fraction", "Fraction of packets to drop");
    static const FrameSimulatorOption<std::string> opt_droppackets("drop-packets", "Packet number(s) to drop");

}

#endif //FRAMESIMULATOR_FRAMESIMULATOROPTIONSUDP_H
