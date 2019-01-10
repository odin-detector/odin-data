#ifndef FRAMESIMULATOR_PCAPFRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_PCAPFRAMESIMULATORPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameSimulatorPlugin.h"
#include "Packet.h"

#include <pcap.h>
#include <arpa/inet.h>

#include <iostream>
#include <sstream>

namespace FrameSimulator {

    /** pcapFrameSimulatorPlugin
     *
     *  Any frame simulator plugin which reads pcap files should inherit from this class
     *  not FrameSimulatorPlugin
     *  'extract_frames' is called on setup: this takes the content of the pcap file and organises it in frames to store
     *  'replay_frames' is called by simulate: this will replay the stored frames
     */
    class pcapFrameSimulatorPlugin : public FrameSimulatorPlugin {

    public:

        pcapFrameSimulatorPlugin();
        ~pcapFrameSimulatorPlugin();

        virtual void populate_options(po::options_description& config);

        virtual bool setup(const po::variables_map& vm);
        virtual void simulate();

    protected:

        static void pkt_callback(u_char *user, const pcap_pkthdr *hdr, const u_char *buffer);

        void prepare_packets(const struct pcap_pkthdr *header, const u_char *buffer);
        int send_packet(const Packet& packet, const int& frame) const;

        virtual void extract_frames(const u_char* data, const int& size) = 0;
        virtual void replay_frames() = 0;

        //Packet gap: insert pause between packet_gap packets
        boost::optional<int> packet_gap;
        //Fraction of packets to drop
        boost::optional<float> drop_frac;
        //List of packets to drop
        boost::optional<std::vector<std::string> > drop_packets;

    private:

        std::vector<struct sockaddr_in> m_addrs;
        int m_socket;

        //Used by send_packet to send each frame to the correct port
        mutable int curr_port_index;
        mutable int curr_frame;

        pcap_t *m_handle;
        char errbuf[PCAP_ERRBUF_SIZE];

        /** Pointer to logger **/
        LoggerPtr logger_;

    };

}

#endif //FRAMESIMULATOR_PCAPFRAMESIMULATORPLUGIN_H
