#include "FrameSimulatorPlugin.h"

namespace FrameSimulator {

    FrameSimulatorPlugin::FrameSimulatorPlugin() {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.FrameSimulatorPlugin");
        logger_->setLevel(Level::getAll());

    }

    void FrameSimulatorPlugin::populate_options(po::options_description& config) {

        LOG4CXX_DEBUG(logger_, "Populating base FrameSimulatorPlugin options");

        opt_ports.add_option_to(config);
        opt_frames.add_option_to(config);
        opt_interval.add_option_to(config);

    }

    bool FrameSimulatorPlugin::setup(const po::variables_map& vm) {

        LOG4CXX_DEBUG(logger_, "Setting up base FrameSimulatorPlugin");

        opt_frames.get_val(vm, replay_frames);
        opt_interval.get_val(vm, replay_interval);

    }

}