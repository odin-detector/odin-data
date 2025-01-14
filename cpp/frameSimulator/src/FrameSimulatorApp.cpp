#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <boost/program_options.hpp>

#include "FrameSimulatorPlugin.h"

#include <memory>
#include <cstdlib>
#include <filesystem>

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

// Basic command line options
static const FrameSimulatorOption<std::string> opt_detector("detector", "Set the detector (Excalibur, Eiger etc.)");
static const FrameSimulatorOption<std::string> opt_libpath("lib-path", "Path to detector plugin library");
static const FrameSimulatorOption<unsigned int> opt_debug_level("debug-level", "Set the debug level");
static const FrameSimulatorOption<std::string> opt_log_config("log-config", "Set the log4cxx logging configuration file");

/** Load requested plugin
 * /param[in] map of command line specified variables
 * /return shared pointer to an instance of the requested FrameSimulatorPlugin  class
 * the 'detector' argument must match the library name prefix i.e. 'lib<detector>FrameSimulatorPlugin.so'
 */
std::shared_ptr <FrameSimulator::FrameSimulatorPlugin> get_requested_plugin(const po::variables_map &vm, LoggerPtr &logger) {

    std::string pluginClass = opt_detector.get_val(vm) + librarySuffix;

    std::filesystem::path libraryPathAndName = std::filesystem::path(opt_libpath.get_val(vm)) /
                                                 std::filesystem::path("lib" + pluginClass + ".so");

    std::shared_ptr <FrameSimulator::FrameSimulatorPlugin> plugin;

    try {
        plugin = OdinData::ClassLoader<FrameSimulator::FrameSimulatorPlugin>::load_class(pluginClass, libraryPathAndName.string());
    }
    catch (std::exception& e){
        LOG4CXX_ERROR(logger, "library not found " << libraryPathAndName.string());
    }

    return plugin;

}

/** Check that str contains suffix
 * /param[in] str - string to test
 * /param[in] str - test suffix
 * /return true if suffix found; else false
 */
static bool has_suffix(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/** Parse command line arguments
 * /param[in] argc - argument count; number of arguments to parse
 * /param[in] argv - one-dimensional string array of command line arguments
 * /param[in] logger - pointer to logging instance
 * /param[in] plugin - pointer to plugin instance
 */
int parse_arguments(int argc, char **argv, po::variables_map &vm, LoggerPtr &logger,
                    std::shared_ptr <FrameSimulator::FrameSimulatorPlugin> &plugin) {

    int rc = 0;

    try {

        // Define generic program arguments

        po::options_description generic("Generic options");
        generic.add_options()
                ("help", "Print help message")
                ("version", "Print version information")
                ("subargs", po::value < std::vector < std::string > > (), "Detector specific arguments");
        opt_detector.add_option_to(generic);
        opt_libpath.add_option_to(generic);
        opt_debug_level.add_option_to(generic);
        opt_log_config.add_option_to(generic);

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

        if (opt_log_config.is_specified(vm)) {
            std::string logconf_fname = opt_log_config.get_val(vm);
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

            po::options_description config("Detector options");

            std::filesystem::path libraryPathAndName = std::filesystem::path(opt_libpath.get_val(vm)) /
                                                         std::filesystem::path(
                                                                 "lib" + detector + librarySuffix + ".so");

            std::string pluginClass = detector + librarySuffix;

            plugin = get_requested_plugin(vm, logger);

            if (plugin) {
                plugin->populate_options(config);
            }

            std::vector <std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);

   //         opts.erase(opts.begin());

            po::store(po::command_line_parser(opts).options(config).run(), vm);

            po::notify(vm);

            if (vm.count("help")) {
                std::cout << "usage: frameSimulator " + detector << " --lib-path <path-to-detector-plugin> "
                          << std::endl << std::endl;
                std::cout << "  --version            Print version information" << std::endl;
                std::cout << "  --" + opt_debug_level.get_argstring() + "        " + opt_debug_level.get_description() << std::endl;
                std::cout << "  --" + opt_log_config.get_argstring() + "          " + opt_log_config.get_description() << std::endl << std::endl;
                std::cout << config << std::endl;
                exit(1);
            }

        }

        // If the command-line help option was given, print help and exit
        else if (vm.count("help")) {
            std::cout << "usage: frameSimulator <detector> --lib-path <path-to-detector-plugin> [options]" << std::endl
                      << std::endl;
            std::cout << "  --version            Print version information" << std::endl;
            std::cout << "  --" + opt_debug_level.get_argstring() + "        " + opt_debug_level.get_description() << std::endl;
            std::cout << "  --" + opt_log_config.get_argstring() + "          " + opt_log_config.get_description() << std::endl;
            exit(1);
        }

        if (vm.count("version")) {
            std::cout << "frameSimulator version " << ODIN_DATA_VERSION_STR << std::endl;
            exit(1);
        }

        if (opt_debug_level.is_specified(vm)) {
            set_debug_level(opt_debug_level.get_val(vm));
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

int main(int argc, char *argv[]) {

    LoggerPtr logger(Logger::getLogger("FS.App"));

    {

        std::shared_ptr <FrameSimulator::FrameSimulatorPlugin> plugin;

        po::variables_map vm;
        parse_arguments(argc, argv, vm, logger, plugin);

        if (!plugin) {
            LOG4CXX_ERROR(logger, "Unable to create simulator plugin, application will terminate");
            return 0;
        }

        LOG4CXX_DEBUG(logger, "finished parsing command line options");
        // Setup plugin from command line arguments
        if (plugin->setup(vm)) {
            plugin->simulate(); // and if successful, run simulation
        }

    }

    return 0;

}
