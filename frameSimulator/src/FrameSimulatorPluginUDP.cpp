#include "FrameSimulatorPluginUDP.h"

#include<string.h>
#include<unistd.h>
#include<functional>

#include <net/ethernet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

#include "FrameSimulatorOptionsUDP.h"

namespace FrameSimulator {

    /** Construct a FrameSimulatorPluginUDP
     * setup an instance of the logger
     * initialises curr_frame and curr_port_index for handling assignment of frames to appropriate ports
     */
    FrameSimulatorPluginUDP::FrameSimulatorPluginUDP() : FrameSimulatorPlugin() {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.FrameSimulatorPluginUDP");
        logger_->setLevel(Level::getAll());

        curr_frame = 0;
        curr_port_index = 0;

    }

    /** Setup frame simulator plugin class from store of command line options
     *
     * \param[in] vm - store of given command line options
     * \return true (false) if required program options are (not) specified
     * and the simulator plugin is not ready to use
     *
     * Derived plugins must additionally call this base method even if overridden
     */
    bool FrameSimulatorPluginUDP::setup(const po::variables_map &vm) {

        // Call base class setup method: extract common options (ports, number of frames etc.)
        FrameSimulatorPlugin::setup(vm);

        LOG4CXX_DEBUG(logger_, "Setting up pcap_loop to read packet(s) from packet capture file");

        // Destination IP is a required argument
        if (!opt_destip.is_specified(vm)) {
            LOG4CXX_ERROR(logger_, "Destination IP address not specified");
            return false;
        }

        pcap_playback_ = opt_pcapfile.is_specified(vm);

        if (!pcap_playback_) {
            LOG4CXX_DEBUG(logger_, "pcap file is not specified: simulating packets");
        }
        else {
            LOG4CXX_DEBUG(logger_, "Replaying pcap file")
        }

        // Optional arguments
        opt_packetgap.get_val(vm, packet_gap);
        opt_dropfrac.get_val(vm, drop_frac);

        if (opt_droppackets.is_specified(vm)) {
            set_optionallist_option(opt_droppackets.get_val(vm), drop_packets);
        }

        std::string dest_ip = opt_destip.get_val(vm);

        std::vector <std::string> dest_ports;
        set_list_option(opt_ports.get_val(vm), dest_ports);

        int num_ports = dest_ports.size();

        LOG4CXX_DEBUG(logger_, "Using destination IP address " + dest_ip);

        // Create socket
        m_socket = socket(PF_INET, SOCK_DGRAM, 0);

        // Setup sockaddr_in for each port and store
        for (int p = 0; p < num_ports; p++) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(atoi(dest_ports[p].c_str()));
            addr.sin_addr.s_addr = inet_addr(dest_ip.c_str());
            LOG4CXX_DEBUG(logger_, "Opening socket on port " + dest_ports[p]);
            m_addrs.push_back(addr);
        }

        if (pcap_playback_) {
            // Open a handle to the pcap file
            m_handle = pcap_open_offline(opt_pcapfile.get_val(vm).c_str(), errbuf);

            if (m_handle == NULL) {
                LOG4CXX_ERROR(logger_, "pcap open failed: " << errbuf);
                return false;
            }

            // Loop over the pcap file to read the frames for replay
            pcap_loop(m_handle, -1, pkt_callback, reinterpret_cast<u_char *>(this));
        } else {
            create_frames(replay_numframes.get());
        }

        return true;

    }

    /** Prepare frame(s) for replay by processing the pcap file
     * /param[in] pcap header
     * /param[in] buffer
     * calls extract_frames which should be implemented by each parent class to extract
     * frames of the appropriate type
     */
    void FrameSimulatorPluginUDP::prepare_packets(const struct pcap_pkthdr *header, const u_char *buffer) {

        LOG4CXX_DEBUG(logger_, "Preparing packet(s)");

        int size = header->len;
        struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));
        struct udphdr *udph = (struct udphdr *) (buffer + iph->ihl * 4 + sizeof(struct ethhdr));
        const u_char *d = &buffer[sizeof(struct ethhdr) + iph->ihl * 4 + sizeof udph];

        int header_size = sizeof(struct ethhdr) + iph->ihl * 4 + sizeof(udph);

        const u_char *data = buffer + header_size;

        this->extract_frames(data, header->len - header_size);

    }

    /** Replay the stored frames
     * called by simulate
     */
    void FrameSimulatorPluginUDP::replay_frames() {

        LOG4CXX_DEBUG(logger_, "Replaying frame(s)");

        int frames_to_replay = replay_numframes ? replay_numframes.get() : frames.size();

        LOG4CXX_DEBUG(logger_, "Replaying frames");
        LOG4CXX_DEBUG(logger_, frames_to_replay);

        int total_frames_sent = 0;
        int total_packets_sent = 0;
        int total_packets_dropped = 0;
        int total_bytes_sent = 0;

        for (int f = 0; f < frames_to_replay; f++) {

            int n = f % frames.size();

            time_t start_time;
            time_t end_time;

            time(&start_time);

            int num_packets = frames[n].packets.size();
            int frame_packets_sent = 0;
            int frame_packets_dropped = 0;
            int frame_bytes_sent = 0;

            LOG4CXX_DEBUG(logger_, "Frame " + boost::lexical_cast<std::string>(n) + " packets: " +
                                   boost::lexical_cast<std::string>(num_packets));

            for (int p = 0; p < num_packets; p++) {

                // If drop fraction specified, decide if packet should be dropped
                if (drop_frac) {
                    if (((double) rand() / RAND_MAX) < drop_frac.get()) {
                        frame_packets_dropped += 1;
                        continue;
                    }
                }

                // If drop list was specified and this packet is in it, drop the packet

                if (drop_packets) {
                    std::vector<std::string> drop_packet_vec = drop_packets.get();
                    if (std::find(drop_packet_vec.begin(), drop_packet_vec.end(), boost::lexical_cast<std::string>(p)) != drop_packet_vec.end()) {
                        frame_packets_dropped += 1;
                        continue;
                    }
                }

                frame_bytes_sent += send_packet(frames[n].packets[p], n);
                frame_packets_sent += 1;

                // Add brief pause between 'packet_gap' frames if packet gap specified
                if (packet_gap && (frame_packets_sent % packet_gap.get() == 0)) {
                    LOG4CXX_DEBUG(logger_,
                                  "Pause - just sent packet - " + boost::lexical_cast<std::string>(frame_packets_sent));
                    sleep(0.01);
                }

            }


            time(&end_time);

            float frame_time_s = difftime(end_time, start_time);

            // Calculate wait time and sleep so that frames are sent at requested intervals
            if (replay_interval) {
                float wait_time_s = replay_interval.get() - frame_time_s;
                if (wait_time_s > 0) {
                    LOG4CXX_DEBUG(logger_,
                                  "Pause after frame " + boost::lexical_cast<std::string>(n));
                    struct timespec wait_spec;
                    wait_spec.tv_sec = (int)wait_time_s;
                    wait_spec.tv_nsec = (long)((wait_time_s - (float)wait_spec.tv_sec) * 1000000000L);
                    nanosleep(&wait_spec, NULL);
                }
            }

            LOG4CXX_DEBUG(logger_, "Sent " + boost::lexical_cast<std::string>(frame_bytes_sent) + " bytes in " +
                                   boost::lexical_cast<std::string>(frame_packets_sent) + " packets for frame " + boost::lexical_cast<std::string>(n) +
                                   ", dropping " + boost::lexical_cast<std::string>(frame_packets_dropped) + " packets (" +
                                   boost::lexical_cast<std::string>(float(100.0*frame_packets_dropped)/(frame_packets_dropped + frame_packets_sent)) + "%)");

            total_frames_sent += 1;
            total_packets_sent += frame_packets_sent;
            total_packets_dropped += frame_packets_dropped;
            total_bytes_sent += frame_bytes_sent;

        }

        LOG4CXX_DEBUG(logger_, "Sent " + boost::lexical_cast<std::string>(total_frames_sent) + " frames with a total of " +
                               boost::lexical_cast<std::string>(total_bytes_sent) + " bytes in " +
                               boost::lexical_cast<std::string>(total_packets_sent) + " packets, dropping " +
                               boost::lexical_cast<std::string>(total_packets_dropped) + " packets (" +
                               boost::lexical_cast<std::string>(float(100.0*total_packets_dropped)/(total_packets_dropped + total_packets_sent)) + "%)");
    }

    /** All Packets should be sent using send_packet
     * /param[in] packet to send
     * /param[in] frame to which packet belongs
     * this ensures each frame is sent to the appropriate destination port
     */
    int FrameSimulatorPluginUDP::send_packet(const boost::shared_ptr<Packet> &packet, const int &frame) const {
        if (frame != curr_frame) {
            curr_port_index = (curr_port_index + 1 < m_addrs.size()) ? curr_port_index + 1 : 0;
            curr_frame = frame;
        }
        bind(m_socket, (struct sockaddr *) (&m_addrs[curr_port_index]), sizeof(m_addrs[curr_port_index]));
        return sendto(m_socket, packet->data, packet->size, 0, (struct sockaddr *) (&m_addrs[curr_port_index]),
                      sizeof(m_addrs[curr_port_index]));
    }

    /** Populate boost program options with appropriate command line options for plugin
     * /param[out] config - boost::program_options::options_description to populate with
     * appropriate plugin command line options
     */
    void FrameSimulatorPluginUDP::populate_options(po::options_description &config) {

        FrameSimulatorPlugin::populate_options(config);

        opt_destip.add_option_to(config);
        opt_pcapfile.add_option_to(config);
        opt_packetgap.add_option_to(config);
        opt_dropfrac.add_option_to(config);
        opt_droppackets.add_option_to(config);

    }

    /** Class destructor
     * closes socket
     */
    FrameSimulatorPluginUDP::~FrameSimulatorPluginUDP() {

        close(m_socket);

    }

    /** Packet callback function
     * callback for pcap_loop(m_handle, -1, pkt_callback, reinterpret_cast<u_char *>(this));
     * /param[in] user - final argument of pcap_loop; this class instance
     * /param[in] hdr - pcap header
     * /param[in] buffer pointer to first byte of chunk of data containing the entire packet
     */
    void FrameSimulatorPluginUDP::pkt_callback(u_char *user, const pcap_pkthdr *hdr, const u_char *buffer) {
        FrameSimulatorPluginUDP *replayer = reinterpret_cast<FrameSimulatorPluginUDP *>(user);
        replayer->prepare_packets(hdr, buffer);
    }

    /** Simulate detector by replaying frames
     *
     */
    void FrameSimulatorPluginUDP::simulate() {

        this->replay_frames();

    }

}
