#ifndef FRAMESIMULATOR_DUMMYUDPFRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_DUMMYUDPFRAMESIMULATORPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <map>
#include <string>
#include <stdexcept>

#include "ClassLoader.h"
#include "FrameSimulatorPluginUDP.h"

namespace FrameSimulator {

    /** DummyUDPFrameSimulatorPlugin
     *
     *  'prepare_packets' (and then 'extract_frames') is called on setup if a pcap file is specified: this takes the
     *  content of the pcap file and organises it in frames to store
     *  'create_frames' is called on setup if no pcap file is specified
     *  'replay_frames' is called by simulate: this will replay the created/stored frames
     */
    class DummyUDPFrameSimulatorPlugin : public FrameSimulatorPluginUDP {

    public:

        DummyUDPFrameSimulatorPlugin();

        virtual void populate_options(po::options_description& config);
        virtual bool setup(const po::variables_map& vm);

        virtual int get_version_major();
        virtual int get_version_minor();
        virtual int get_version_patch();
        virtual std::string get_version_short();
        virtual std::string get_version_long();

    protected:

        virtual void extract_frames(const u_char* data, const int& size);
        virtual void create_frames(const int &num_frames);

    private:

        /** Pointer to logger **/
        LoggerPtr logger_;

        int image_width_;
        int image_height_;
        int packet_len_;

    };

}

#endif //FRAMESIMULATOR_DUMMYUDPSIMULATORPLUGIN_H
