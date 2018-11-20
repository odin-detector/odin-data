#include "pcapFrameSimulatorPlugin.h"

#include<string.h>
#include<unistd.h>
#include<functional>

#include <net/ethernet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "pcapFrameSimulatorOptions.h"

namespace FrameSimulator {

    pcapFrameSimulatorPlugin::pcapFrameSimulatorPlugin() : FrameSimulatorPlugin() {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.pcapFrameSimulatorPlugin");
        logger_->setLevel(Level::getAll());

        curr_frame = 0;
        curr_port_index = 0;

    }

    bool pcapFrameSimulatorPlugin::setup(const po::variables_map& vm) {

        FrameSimulatorPlugin::setup(vm);

        LOG4CXX_DEBUG(logger_, "Setting up pcap_loop to read packet(s) from packet capture file");

        if (!opt_ports.is_specified(vm)) {
            LOG4CXX_ERROR(logger_, "Destination ports not specified");
            return false;
        }

        if (!opt_destip.is_specified(vm)) {
            LOG4CXX_ERROR(logger_, "Destination IP address not specified");
            return false;
        }

        if (!opt_pcapfile.is_specified(vm)) {
            LOG4CXX_ERROR(logger_, "pcap file is not specified");
            return false;
        }

        opt_packetgap.get_val(vm, packet_gap);
        opt_dropfrac.get_val(vm, drop_frac);

        if (opt_droppackets.is_specified(vm)) {
            std::vector<std::string> drop_packets_vec;
            std::string drop_packets_string = opt_droppackets.get_val(vm);
            boost::split(drop_packets_vec, drop_packets_string, boost::is_any_of(","), boost::token_compress_on);
            drop_packets = drop_packets_vec;
        }

        std::string dest_ip = opt_destip.get_val(vm);

        std::vector<std::string> dest_ports;
        std::string dest_ports_string = opt_ports.get_val(vm);
        boost::split(dest_ports, dest_ports_string, boost::is_any_of(","), boost::token_compress_on);

        int num_ports = dest_ports.size();

        LOG4CXX_DEBUG(logger_, "Using destination IP address " + dest_ip);

        m_socket = socket(PF_INET, SOCK_DGRAM, 0);

        for (int p=0; p<num_ports; p++) {

            struct sockaddr_in addr;

            memset(&addr, 0, sizeof(addr));

            addr.sin_family = AF_INET;
            addr.sin_port = htons(atoi(dest_ports[p].c_str()));
            addr.sin_addr.s_addr = inet_addr(dest_ip.c_str());

            LOG4CXX_DEBUG(logger_, "Opening socket on port " + dest_ports[p]);

            m_addrs.push_back(addr);

        }

        m_handle = pcap_open_offline(opt_pcapfile.get_val(vm).c_str(), errbuf);

        if (m_handle == NULL) {
            LOG4CXX_ERROR(logger_, "pcap open failed: " << errbuf);
            return false;
        }

        pcap_loop(m_handle, -1, pkt_callback, reinterpret_cast<u_char *>(this));

    }

    void pcapFrameSimulatorPlugin::prepare_packets(const struct pcap_pkthdr *header, const u_char *buffer) {

        LOG4CXX_DEBUG(logger_, "Preparing packet(s)");

        int size = header->len;
        struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
        struct udphdr *udph = (struct udphdr *) (buffer + iph->ihl * 4 + sizeof(struct ethhdr));
        const u_char *d = &buffer[sizeof(struct ethhdr) + iph->ihl * 4 + sizeof udph];

        int header_size = sizeof(struct ethhdr) + iph->ihl * 4 + sizeof(udph);

        const u_char *data = buffer + header_size;

        this->extract_frames(data, header->len - header_size);

    }

    int pcapFrameSimulatorPlugin::send_packet(const Packet& packet, const int& frame) const {
        if (frame != curr_frame) {
            curr_port_index = (curr_port_index + 1 < m_addrs.size()) ? curr_port_index + 1 : 0;
            curr_frame = frame;
        }
        bind(m_socket, (struct sockaddr *) (&m_addrs[curr_port_index]), sizeof(m_addrs[curr_port_index]));
        return sendto(m_socket, packet.data, packet.size, 0, (struct sockaddr *) (&m_addrs[curr_port_index]), sizeof(m_addrs[curr_port_index]));
    }

    void pcapFrameSimulatorPlugin::populate_options(po::options_description& config) {

        FrameSimulatorPlugin::populate_options(config);

        opt_destip.add_option_to(config);
        opt_pcapfile.add_option_to(config);
        opt_packetgap.add_option_to(config);
        opt_dropfrac.add_option_to(config);
        opt_droppackets.add_option_to(config);

    }

    pcapFrameSimulatorPlugin::~pcapFrameSimulatorPlugin() {

        close(m_socket);

    }

    void pcapFrameSimulatorPlugin::pkt_callback(u_char *user, const pcap_pkthdr *hdr, const u_char *buffer) {
        pcapFrameSimulatorPlugin* replayer = reinterpret_cast<pcapFrameSimulatorPlugin*>(user);
        replayer->prepare_packets(hdr, buffer);
    }

    void pcapFrameSimulatorPlugin::simulate() {

        this->replay_frames();

    }

}