#ifndef FRAMESIMULATOR_FRAMESIMULATOROPTIONS_H
#define FRAMESIMULATOR_FRAMESIMULATOROPTIONS_H

#include <boost/optional.hpp>
#include <vector>
#include <string>

namespace FrameSimulator {

    struct FrameSimulatorOptions {

        FrameSimulatorOptions();

        boost::optional<std::string> dest_ports; // Port number(s) to transmit frame data to
        boost::optional<int> num_frames; // Number of frames to transmit; if unset send all available
        boost::optional<int> interval; // Interval in seconds between transmission of frames; if unset assume no interval

        // Pcap frame simulator specific option(s)

        boost::optional<std::string> dest_ip_addr; // Hostname or IP address to transmit data to
        boost::optional<std::string> pcap_file; // Packet capture file to load
        boost::optional<int> pkt_gap; // Insert brief pause between every N packets
        boost::optional<float> drop_fraction; // Fraction of packets to drop
        boost::optional<std::string> drop_packets; // Packet number(s) to drop from each frame

        // Eiger specific option(s)

        boost::optional<std::string> acquisition_id; // Acquisition ID
        boost::optional<std::string> filepattern; // File pattern of frame file(s)
        boost::optional<int> delay_adjustment; // Optional delay adjustment

    };

}

#endif