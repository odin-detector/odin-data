#include <iostream>
#include <string>
#include <map>
#include <boost/program_options.hpp>

#include "FrameSimulatorPlugin.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "ClassLoader.h"

#include "FrameSimulatorOptions.h"
#include "FrameSimulatorOption.h"

#include "logging.h"
#include "DebugLevelLogger.h"
#include "version.h"

using namespace FrameSimulator;

namespace po = boost::program_options;

static const FrameSimulatorOption<std::string> opt_detector("detector", "Set the detector (Excalibur, Eiger etc.)");
static const FrameSimulatorOption<std::string> opt_libpath("lib-path", "Path to detector plugin library");
static const FrameSimulatorOption<unsigned int> opt_debuglevel("debug-level", "Set the debug level");
static const FrameSimulatorOption<std::string> opt_logconfig("logconfig", "Set the log4cxx logging configuration file");

static const FrameSimulatorOption<std::string> opt_ports("ports", "Provide a (comma separated) port list");
static const FrameSimulatorOption<int> opt_frames("frames", "Number of frames (-1 sends all frames)");
static const FrameSimulatorOption<int> opt_interval("interval", "Transmission interval between frames");
static const FrameSimulatorOption<std::string> opt_destip("dest-ip", "Destination IP address");
static const FrameSimulatorOption<std::string> opt_pcapfile("pcap-file", "Packet capture file");
static const FrameSimulatorOption<int> opt_packetgap("packet-gap", "Pause between N packets");
static const FrameSimulatorOption<float> opt_dropfrac("drop-fraction", "Fraction of packets to drop");
static const FrameSimulatorOption<std::string> opt_droppackets("drop-packets", "Packet number(s) to drop");
static const FrameSimulatorOption<std::string> opt_acqid("acquisition-id", "Acquisition ID");
static const FrameSimulatorOption<std::string> opt_filepattern("file-pattern", "File pattern");
static const FrameSimulatorOption<int> opt_delay("delay-adjustment", "Delay adjustment");

static bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int parse_arguments(int argc, char** argv, po::variables_map& vm, LoggerPtr& logger) {

    int rc = 0;

    try {

        po::options_description generic("Generic options");
        generic.add_options()
                ("help", "Print help message");
        opt_detector.add_option_to(generic);
        opt_libpath.add_option_to(generic);

        po::options_description config("Configuration options");

        opt_debuglevel.add_option_to(config);
        opt_logconfig.add_option_to(config);
        opt_ports.add_option_to(config);
        opt_frames.add_option_to(config);
        opt_interval.add_option_to(config);
        opt_destip.add_option_to(config);
        opt_pcapfile.add_option_to(config);
        opt_packetgap.add_option_to(config);
        opt_dropfrac.add_option_to(config);
        opt_droppackets.add_option_to(config);
        opt_acqid.add_option_to(config);
        opt_filepattern.add_option_to(config);
        opt_delay.add_option_to(config);

        // Group the variables for parsing at the command line and/or from the configuration file
        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config);
        po::options_description config_file_options;
        config_file_options.add(config);

        // Parse the command line options
        po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
        po::notify(vm);

        // If the command-line help option was given, print help and exit
        if (vm.count("help")) {
            std::cout << "usage: frameSimulator [options]" << std::endl << std::endl;
            std::cout << "       odin-data version: " << ODIN_DATA_VERSION_STR << std::endl << std::endl;
            std::cout << cmdline_options << std::endl;
            exit(1);
        }

        if (opt_logconfig.is_specified(vm)) {
            std::string logconf_fname = opt_logconfig.get_val(vm);
            if (has_suffix(logconf_fname, ".xml")) {
                log4cxx::xml::DOMConfigurator::configure(logconf_fname);
            } else {
                PropertyConfigurator::configure(logconf_fname);
            }
            LOG4CXX_DEBUG(logger, "log4cxx config file is set to " << logconf_fname);
        } else {
            BasicConfigurator::configure();
        }

        if (opt_debuglevel.is_specified(vm)) {
            set_debug_level(opt_debuglevel.get_val(vm));
            LOG4CXX_DEBUG_LEVEL(1, logger, "Debug level set to  " << debug_level);
        }

        if (!opt_detector.is_specified(vm)) {
            LOG4CXX_ERROR(logger, "Detector not specified, exiting.");
            exit(1);
        }

        if (!opt_libpath.is_specified(vm)) {
            LOG4CXX_ERROR(logger, "Detector library path not specified, exiting.");
            exit(1);
        }

    }

    catch (...) {
        std::cout << "Exception parsing arguments.";
        rc = 1;
    }

    return rc;

}

int main(int argc, char* argv[]) {

    LoggerPtr logger(Logger::getLogger("FS.App"));

    try {

        po::variables_map vm;
        parse_arguments(argc, argv, vm, logger);

        boost::filesystem::path libraryPathAndName = boost::filesystem::path(vm["lib-path"].as<std::string>()) /
                                                     boost::filesystem::path("lib" + vm["detector"].as<std::string>() +
                                                                             "FrameSimulatorPlugin.so");

        std::string pluginClass = vm["detector"].as<std::string>() + "FrameSimulatorPlugin";

        boost::shared_ptr<FrameSimulator::FrameSimulatorPlugin> plugin = OdinData::ClassLoader<FrameSimulator::FrameSimulatorPlugin>::load_class(
                pluginClass, libraryPathAndName.string());

        if (!plugin)
            LOG4CXX_ERROR(logger, "Unable to create simulator plugin, application will terminate");

        FrameSimulatorOptions opts;

        opt_ports.get_val(vm, opts.dest_ports);
        opt_frames.get_val(vm, opts.num_frames);
        opt_interval.get_val(vm, opts.interval);
        opt_destip.get_val(vm, opts.dest_ip_addr);
        opt_pcapfile.get_val(vm, opts.pcap_file);
        opt_packetgap.get_val(vm, opts.pkt_gap);
        opt_dropfrac.get_val(vm, opts.drop_fraction);
        opt_droppackets.get_val(vm, opts.drop_packets);
        opt_acqid.get_val(vm, opts.acquisition_id);
        opt_filepattern.get_val(vm, opts.filepattern);
        opt_delay.get_val(vm, opts.delay_adjustment);

        if (plugin->setup(opts))
            plugin->simulate();

    } catch (const std::exception &e) {
        //LOG4CXX_ERROR(logger, "Caught unhandled exception in FrameSimulator, application will terminate: " << e.what());
        throw;
    }

    return 0;

}