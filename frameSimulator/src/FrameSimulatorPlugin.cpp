#include "FrameSimulatorPlugin.h"

namespace FrameSimulator {

    FrameSimulatorPlugin::FrameSimulatorPlugin() {


    }

    void FrameSimulatorPlugin::populate_options(po::options_description& config) {

        opt_ports.add_option_to(config);
        opt_frames.add_option_to(config);
        opt_interval.add_option_to(config);

    }

}