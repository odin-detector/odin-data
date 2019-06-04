#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <signal.h>

#include <boost/program_options.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>

#include "FrameSimulatorControl.h"
#include "FrameReceiverControl.h"
#include "FrameProcessorControl.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

using namespace FrameSimulatorTest;

namespace po = boost::program_options;

#include "logging.h"
#include "DebugLevelLogger.h"

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
 * /param[in] ptree - property tree instance
 */
int parse_arguments(int argc,
                    char **argv, po::variables_map &vm,
                    LoggerPtr &logger,
                    boost::property_tree::ptree &ptree) {

  int rc = 0;

  try {

    po::options_description generic("Generic options");

    generic.add_options()
            ("ini", po::value<std::string>(), "Configuration file")
            ("help", "Print help message");

    po::options_description cmdline_options;
    cmdline_options.add(generic);

    po::parsed_options parsed = po::command_line_parser(argc, argv).
            options(generic).
            allow_unregistered().
            run();

    po::store(parsed, vm);

    // If the command-line help option was given, print help and exit

    if (vm.count("help")) {
      std::cout << "usage: FrameTest [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      exit(1);
    }

    // Setup logging

    if (vm.count("logconfig")) {
      std::string logconf_fname = vm["logconfig"].as<std::string>();
      if (has_suffix(logconf_fname, ".xml")) {
        log4cxx::xml::DOMConfigurator::configure(logconf_fname);
      } else {
        PropertyConfigurator::configure(logconf_fname);
      }
      LOG4CXX_DEBUG(logger, "log4cxx config file is set to " << logconf_fname);
    } else {
      BasicConfigurator::configure();
    }

    // Read configuration file; store as boost::property_tree::ptree

    if (vm.count("ini")) {
      std::string config_file = vm["ini"].as<std::string>();
      LOG4CXX_DEBUG(logger, "Reading config file " << config_file);
      boost::property_tree::ini_parser::read_ini(config_file, ptree);
    } else {
      LOG4CXX_ERROR(logger, "No configuration file specified. Exiting.");
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

  LoggerPtr logger(Logger::getLogger("Test.App"));

  try {

    int status;

    // Process IDs for receiver, processor and simulator processes
    pid_t receiver_pid = -1;
    pid_t processor_pid = -1;
    pid_t simulator_pid = -1;

    // Read command arguments into pt
    boost::property_tree::ptree pt;
    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger, pt);

    // Create a receiver, processor and simulator with properties in pt
    FrameReceiverControl receiver(pt, receiver_pid, logger);
    FrameProcessorControl processor(pt, processor_pid, logger);
    FrameSimulatorControl simulator(pt.get<std::string>("Main.detector"), pt, simulator_pid, logger);

    // Run each process; main program to wait for simulator to complete
    receiver.run_process();
    processor.run_process();
    simulator.run_process(true);

    // Allow receiver and processor time to finish frame collection
    sleep(5);

    // If receiver hasn't exited with error; interrupt
    if (receiver_pid != -1) {
      kill(receiver_pid, SIGINT);
    }

    // If processor hasn't exited with error; interrupt
    if (processor_pid != -1) {
      kill(processor_pid, SIGINT);
    }

  } catch (const std::exception &e) {
    LOG4CXX_ERROR(logger, "Caught unhandled exception in FrameTestApp, application will terminate: " << e.what());
    throw;
  }

  return 0;

}
