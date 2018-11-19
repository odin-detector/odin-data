#include <iostream>
#include <string>
#include <map>
#include <vector>
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

static const std::string librarySuffix = "FrameSimulatorPlugin";

static const FrameSimulatorOption<std::string> opt_detector("detector", "Set the detector (Excalibur, Eiger etc.)");
static const FrameSimulatorOption<std::string> opt_libpath("lib-path", "Path to detector plugin library");
static const FrameSimulatorOption<unsigned int> opt_debuglevel("debug-level", "Set the debug level");
static const FrameSimulatorOption<std::string> opt_logconfig("logconfig", "Set the log4cxx logging configuration file");

static const FrameSimulatorOption<std::string> opt_acqid("acquisition-id", "Acquisition ID");
static const FrameSimulatorOption<std::string> opt_filepattern("file-pattern", "File pattern");
static const FrameSimulatorOption<int> opt_delay("delay-adjustment", "Delay adjustment");

boost::shared_ptr<FrameSimulator::FrameSimulatorPlugin> get_requested_plugin(const po::variables_map& vm) {

    std::string pluginClass = opt_detector.get_val(vm) + librarySuffix;

    boost::filesystem::path libraryPathAndName = boost::filesystem::path(opt_libpath.get_val(vm)) /
        boost::filesystem::path("lib" + pluginClass + ".so");

boost::shared_ptr<FrameSimulator::FrameSimulatorPlugin> plugin = \
    OdinData::ClassLoader<FrameSimulator::FrameSimulatorPlugin>::load_class(pluginClass, libraryPathAndName.string());

    return plugin;

}

static bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int parse_arguments(int argc, char** argv, po::variables_map& vm, LoggerPtr& logger,
                    boost::shared_ptr<FrameSimulator::FrameSimulatorPlugin>& plugin) {

    int rc = 0;

    try {

        // Define generic program arguments

        po::options_description generic("Generic options");
        generic.add_options()
                ("help", "Print help message")
                ("subargs", po::value<std::vector<std::string> >(), "Detector specific arguments");
        opt_detector.add_option_to(generic);
        opt_libpath.add_option_to(generic);

        po::positional_options_description pos;
        pos.add(opt_detector.get_arg(), 1).add("subargs", -1);

        // Group the variables for parsing at the command line

        po::options_description cmdline_options;
        cmdline_options.add(generic);

        po::parsed_options parsed = po::command_line_parser(argc, argv).
                options(generic).
                positional(pos).
                allow_unregistered().
                run();

        po::store(parsed, vm);

        // Setup logging

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

        //Parse detector specific arguments

        if (opt_detector.is_specified(vm)) {

            if (!opt_libpath.is_specified(vm)) {
                LOG4CXX_ERROR(logger, "Detector library path not specified, exiting.");
                exit(1);
            }

            std::string detector = opt_detector.get_val(vm);

            po::options_description config("Configuration options");

            boost::filesystem::path libraryPathAndName = boost::filesystem::path(opt_libpath.get_val(vm)) /
                                                         boost::filesystem::path("lib"+detector+librarySuffix+".so");

            std::string pluginClass = detector + librarySuffix;

            plugin = get_requested_plugin(vm);

            if (plugin)
                plugin->populate_options(config);

            std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);

            opts.erase(opts.begin());

            po::store(po::command_line_parser(opts).options(config).run(), vm);

            po::notify(vm);

            if (vm.count("help")) {
                std::cout << "usage: frameSimulator " + detector << " --lib-path <path-to-detector-plugin> "<<std::endl << std::endl;
                std::cout << "       odin-data version: " << ODIN_DATA_VERSION_STR << std::endl << std::endl;
                std::cout << config << std::endl;
                exit(1);
            }

        }

        // If the command-line help option was given, print help and exit
        else if (vm.count("help")) {
            std::cout << "usage: frameSimulator <detector> --lib-path <path-to-detector-plugin> [options]" << std::endl << std::endl;
            std::cout << "       odin-data version: " << ODIN_DATA_VERSION_STR << std::endl << std::endl;
            exit(1);
        }

        if (opt_debuglevel.is_specified(vm)) {
            set_debug_level(opt_debuglevel.get_val(vm));
            LOG4CXX_DEBUG_LEVEL(1, logger, "Debug level set to  " << debug_level);
        }

        if (!opt_detector.is_specified(vm)) {
            LOG4CXX_ERROR(logger, "Detector not specified, exiting.");
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

        boost::shared_ptr<FrameSimulator::FrameSimulatorPlugin> plugin;

        po::variables_map vm;
        parse_arguments(argc, argv, vm, logger, plugin);

        if (!plugin)
            LOG4CXX_ERROR(logger, "Unable to create simulator plugin, application will terminate");

        if (plugin->setup(vm))
            plugin->simulate();


    } catch (const std::exception &e) {
        //LOG4CXX_ERROR(logger, "Caught unhandled exception in FrameSimulator, application will terminate: " << e.what());
        throw;
    }

    return 0;

}