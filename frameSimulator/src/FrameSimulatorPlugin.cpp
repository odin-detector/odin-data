#include "FrameSimulatorPlugin.h"

namespace FrameSimulator {

    FrameSimulatorPlugin::FrameSimulatorPlugin() {

        // Setup logging for the class
        logger_ = Logger::getLogger("FS.FrameSimulatorPlugin");
        logger_->setLevel(Level::getAll());

    }

    /** Populate boost program options with appropriate command line options for plugin
     *
     * \param[out] config - boost::program_options::options_description to populate with
     * appropriate plugin command line options
     */
    void FrameSimulatorPlugin::populate_options(po::options_description &config) {

        LOG4CXX_DEBUG(logger_, "Populating base FrameSimulatorPlugin options");

        opt_ports.add_option_to(config);
        opt_frames.add_option_to(config);
        opt_interval.add_option_to(config);

    }

    /** Setup frame simulator plugin class from store of command line options
     *
     * \param[in] vm - store of given command line options
     * \return true (false) if required program options are (not) specified
     * and the simulator plugin is not ready to use
     *
     * Derived plugins must additionally call this base method even if overridden
     */
    bool FrameSimulatorPlugin::setup(const po::variables_map &vm) {

        LOG4CXX_DEBUG(logger_, "Setting up base FrameSimulatorPlugin");

        // Port(s) must be specified for all plugin classes
        if (!opt_ports.is_specified(vm)) {
            LOG4CXX_ERROR(logger_, "Destination ports not specified");
            return false;
        }

        opt_frames.get_val(vm, replay_numframes);
        opt_interval.get_val(vm, replay_interval);

        return true;

    }

}
