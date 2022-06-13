#include <string>

#include "DummyUDPFrameSimulatorPlugin.h"
#include "DummyUDPDefinitions.h"
#include "FrameSimulatorOptionsDummyUDP.h"
#include "DummyUDPFrame.h"

#include <cstdlib>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "version.h"

namespace FrameSimulator {

    /** Construct an DummyUDPFrameSimulatorPlugin
    * setup an instance of the logger
    * initialises data and frame counts
    */
    DummyUDPFrameSimulatorPlugin::DummyUDPFrameSimulatorPlugin() : 
        FrameSimulatorPluginUDP(),
        image_width_(default_image_width),
        image_height_(default_image_height),
        packet_len_(default_pkt_len)
    {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.DummyUDPFrameSimulatorPlugin");
        logger_->setLevel(Level::getAll());

        total_packets = 0;
        total_bytes = 0;

        current_frame_num = -1;
        current_subframe_num = -1;

    }

    void DummyUDPFrameSimulatorPlugin::populate_options(po::options_description& config)
    {
        FrameSimulatorPluginUDP::populate_options(config);

        opt_image_width.add_option_to(config);
        opt_image_height.add_option_to(config);
        opt_packet_len.add_option_to(config);
    }
    
    bool DummyUDPFrameSimulatorPlugin::setup(const po::variables_map& vm)
    {
        LOG4CXX_DEBUG(logger_, "Setting up DummyUDP simulator plugin");

        // Extract optional arguments for this plugin
        boost::optional<int> image_width;
        boost::optional<int> image_height;
        boost::optional<int> packet_len;

        opt_image_width.get_val(vm, image_width);
        if (image_width) {
            image_width_ = image_width.get();
        }
        opt_image_height.get_val(vm, image_height);
        if (image_height) {
            image_height_ = image_height.get();
        }
        opt_packet_len.get_val(vm, packet_len);
        if (packet_len) {
            packet_len_ = packet_len.get();
        }

        LOG4CXX_DEBUG(logger_, "Using image width: " << image_width_ 
            << " height: " << image_height_
            << " packet length: " << packet_len_);

        return FrameSimulatorPluginUDP::setup(vm);

    }

    /** Creates a number of frames
     *
     * @param num_frames - number of frames to create
     */
    void DummyUDPFrameSimulatorPlugin::create_frames(const int &num_frames) {

        LOG4CXX_DEBUG(logger_, "Creating frame(s)");

        // Build array of pixel data used for eachframe
        const int num_pixels = image_width_ * image_height_;
        uint16_t* pixel_data = new uint16_t[num_pixels];
        for (int pixel = 0; pixel < num_pixels; pixel++) 
        {
            pixel_data[pixel] = static_cast<uint16_t>(pixel & 0xFFFF);
        }

        // Calculate number of pixel image bytes in frame
        std::size_t image_bytes = num_pixels * sizeof(uint16_t);

        // Allocate buffer for packet data including header
        u_char* packet_data = new u_char[packet_len_ + sizeof(DummyUDPPacketHeader)];

        // Loop over specified number of frames to generate packet and frame data
        for (int frame = 0; frame < num_frames; frame++)
        {
            
            u_char* data_ptr = reinterpret_cast<u_char*>(pixel_data);

            std::size_t bytes_remaining = image_bytes;
            uint32_t packet_number = 0;

            while (bytes_remaining > 0) {
            
                std::size_t current_packet_len = 0;
                uint32_t packet_flags = 0;
            
                // Determine packet length and header flags based on current state
                if (bytes_remaining > packet_len_)
                {
                    current_packet_len = packet_len_;
                    if (packet_number == 0) 
                    {
                       packet_flags = DummyUDP::start_of_frame_mask;
                    }
                }
                else 
                {
                    current_packet_len = bytes_remaining;
                    packet_flags = DummyUDP::end_of_frame_mask;
                }

                // Get a pointer to the packet header and fill in fields appropriately.
                DummyUDPPacketHeader* packet_hdr = 
                    reinterpret_cast<DummyUDPPacketHeader*>(packet_data);

                packet_hdr->frame_number = frame;
                packet_hdr->packet_number_flags = 
                    (packet_number & DummyUDP::packet_number_mask) | packet_flags;

                // Copy the appropriate region of image data into the packet buffer
                memcpy((packet_data + sizeof(DummyUDPPacketHeader)), data_ptr, current_packet_len);

                // Pass packet to frame extraction
                this->extract_frames(packet_data, current_packet_len + sizeof(DummyUDPPacketHeader));

                // Update packet number, decreased bytes remaining and update image data pointer
                packet_number++;
                bytes_remaining -= current_packet_len;
                data_ptr += current_packet_len;
            }

        }

        delete [] packet_data;
        delete [] pixel_data;

    }


    /** Extracts the frames from the pcap data file buffer
     * \param[in] data - pcap data
     * \param[in] size
     */
    void DummyUDPFrameSimulatorPlugin::extract_frames(const u_char *data, const int &size) {

        // Map a packet header onto the start of the data buffer and extract fields
        const DummyUDPPacketHeader* packet_hdr = reinterpret_cast<const DummyUDPPacketHeader*>(data);

        uint32_t frame_number = packet_hdr->frame_number;
        uint32_t packet_number_flags = packet_hdr->packet_number_flags;

        bool is_SOF = packet_number_flags & DummyUDP::start_of_frame_mask;
        bool is_EOF = packet_number_flags & DummyUDP::end_of_frame_mask;
        uint32_t packet_number = packet_number_flags & DummyUDP::packet_number_mask;

        if (is_SOF)
        {
            LOG4CXX_DEBUG(logger_, "SOF marker for frame " << frame_number 
                << " at packet " << packet_number << " total " << total_packets);

            if (packet_number != 0) {
                LOG4CXX_WARN(logger_, "Detected SOF marker on packet number != 0");
            }

            UDPFrame frame(frame_number);
            frames_.push_back(frame);
            frames_[frames_.size() - 1].SOF_markers.push_back(frame_number);

        }

        if (is_EOF)
        {
            LOG4CXX_DEBUG(logger_, "EOF marker for frame " << frame_number 
                << " at packet " << packet_number << " total " << total_packets);
                frames_[frames_.size() - 1].EOF_markers.push_back(frame_number);
        }

        // Allocate a new packet, copy packet data and push into frame
        boost::shared_ptr<Packet> pkt(new Packet());
        unsigned char *datacp = new unsigned char[size];
        memcpy(datacp, data, size);
        pkt->data = datacp;
        pkt->size = size;
        frames_[frames_.size() - 1].packets.push_back(pkt);

        total_packets++;

    }

   /**
    * Get the plugin major version number.
    *
    * \return major version number as an integer
    */
    int DummyUDPFrameSimulatorPlugin::get_version_major() {
        return ODIN_DATA_VERSION_MAJOR;
    }

    /**
     * Get the plugin minor version number.
     *
     * \return minor version number as an integer
     */
    int DummyUDPFrameSimulatorPlugin::get_version_minor() {
        return ODIN_DATA_VERSION_MINOR;
    }

    /**
     * Get the plugin patch version number.
     *
     * \return patch version number as an integer
     */
    int DummyUDPFrameSimulatorPlugin::get_version_patch() {
        return ODIN_DATA_VERSION_PATCH;
    }

    /**
     * Get the plugin short version (e.g. x.y.z) string.
     *
     * \return short version as a string
     */
    std::string DummyUDPFrameSimulatorPlugin::get_version_short() {
        return ODIN_DATA_VERSION_STR_SHORT;
    }

    /**
     * Get the plugin long version (e.g. x.y.z-qualifier) string.
     *
     * \return long version as a string
     */
    std::string DummyUDPFrameSimulatorPlugin::get_version_long() {
        return ODIN_DATA_VERSION_STR;
    }

}
