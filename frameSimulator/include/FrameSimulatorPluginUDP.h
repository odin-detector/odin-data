#ifndef FRAMESIMULATOR_FRAMESIMULATORPLUGINUDP_H
#define FRAMESIMULATOR_FRAMESIMULATORPLUGINUDP_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>

#include "FrameSimulatorPlugin.h"
#include "UDPFrame.h"
#include "Packet.h"

#include <pcap.h>
#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <list>

namespace FrameSimulator {

    typedef std::vector<UDPFrame> UDPFrames;

    /** FrameSimulatorPluginUDP
     *
     *  Any frame simulator plugin which reads pcap or creates dummy frames to send over a socket should inherit from this class
     *  not FrameSimulatorPlugin
     *  'prepare_packets' (and then 'extract_frames') is called on setup if a pcap file is specified: this takes the
     *  content of the pcap file and organises it in frames to store
     *  'create_frames' is called on setup if no pcap file is specified
     *  'replay_frames' is called by simulate: this will replay the created/stored frames
     */
    class FrameSimulatorPluginUDP : public FrameSimulatorPlugin {

    public:

        FrameSimulatorPluginUDP();
        ~FrameSimulatorPluginUDP();

        virtual void populate_options(po::options_description& config);

        virtual bool setup(const po::variables_map& vm);
        virtual void simulate();

    protected:

        static void pkt_callback(u_char *user, const pcap_pkthdr *hdr, const u_char *buffer);
        int send_packet(const boost::shared_ptr<Packet>& packet, const int& frame) const;

        /** Extract frames from pcap read data **/
        virtual void extract_frames(const u_char* data, const int& size) = 0;

        /** Prepare frames from pcap header and data buffer **/
        void prepare_packets(const struct pcap_pkthdr *header, const u_char *buffer);

        /** Create dummy frames **/
        virtual void create_frames(const int &num_frames) = 0;

        /** Replay the extracted or created frames **/
        void replay_frames();

        //Packet gap: pause between packet_gap packets; must be >0
        boost::optional<int> packet_gap_;
        // proportion in [0.0,1.0] of packets to randomly drop
        boost::optional<float> drop_frac_;
        //List of packets to drop, these are simple ints held as strings. 0=first packet etc.
        boost::optional<std::vector<std::string> > drop_packets_;

        /** Frames **/
        UDPFrames frames_;

        int total_packets;
        int total_bytes;

        int current_frame_num;
        int current_subframe_num;

    private:

        struct Target
        {
            sockaddr_in m_addr;
            std::list<Packet> m_packetsToSend;
            void queuePacket(Packet pkt)
            {
                m_packetsToSend.push_back(pkt);
            }
            void SendPackets(FrameSimulatorPluginUDP* pFrameSim);
        };
        std::vector<Target> m_targets;
        int m_socket;

        //Used by send_packet to send each frame to the correct port
        mutable int curr_port_index;
        mutable int curr_frame;

        pcap_t *m_handle;
        char errbuf[PCAP_ERRBUF_SIZE];

        /** Pointer to logger **/
        LoggerPtr logger_;

        /** Replay frames from pcap file **/
        bool pcap_playback_;

    };

}

#endif //FRAMESIMULATOR_FRAMESIMULATORPLUGINUDP_H
