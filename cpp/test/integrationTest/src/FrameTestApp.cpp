#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <signal.h>

#include <boost/program_options.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ControlUtility.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

using namespace FrameSimulatorTest;

namespace po = boost::program_options;

#include "logging.h"
#include "DebugLevelLogger.h"

/** Check that str contains suffix
 * /param[in] str - string to test
 * /param[in] suffix - test suffix
 * /return true if suffix found; else false
 */
static bool has_suffix(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/** Parse command line arguments
 * /param[in] argc - argument count; number of arguments to parse
 * /param[in] argv - one-dimensional string array of command line arguments
 * /param[in] vm - boost program_options variables_map
 * /param[in] logger - pointer to logging instance
 * /param[in] ptree - property tree instance
 */
int parse_arguments(int argc,
                    char **argv,
                    po::variables_map &vm,
                    LoggerPtr &logger,
                    boost::property_tree::ptree &ptree) {

  int rc = 0;

  try {

    po::options_description generic("Generic options");

    generic.add_options()
            ("json", po::value<std::string>(), "Configuration file")
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

    BasicConfigurator::configure();

    // Read configuration file; store as boost::property_tree::ptree

    if (vm.count("json")) {
      std::string config_file = vm["json"].as<std::string>();
      LOG4CXX_DEBUG(logger, "Reading config file " << config_file);
      boost::property_tree::json_parser::read_json(config_file, ptree);
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

    // Process IDs
    std::vector<pid_t> process_pids;
    std::vector<ControlUtility*> processes;

    // Read command arguments into pt
    boost::property_tree::ptree pt;
    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger, pt);

    // Setup ControlUtility instances
    std::vector<ControlUtility*> utilities;
    BOOST_FOREACH(boost::property_tree::ptree::value_type &vc, pt.get_child("Main")) {
      std::string process = vc.first;
      LOG4CXX_INFO(logger, "Process to launch: " + process);
      std::string command_entry = "Main." + process + "." + "command";
      std::string pos_args = pt.get<std::string>("Main." + process + "." + "pos-args");
      std::string socket_entry = "Main." + process + "." + "socket";
      std::string kill_entry = "Main." + process + "." + "kill";
      int sleeptime = pt.get<int>("Main." + process + "." + "sleep");
      ControlUtility* control = new ControlUtility(pt, pos_args, command_entry, process, socket_entry, kill_entry, logger);
      utilities.push_back(control);
      if (pt.get<bool>("Main." + process +"." + "process")) {
        processes.push_back(control);
        control->run_process();
        boost::optional<std::string> configuration = pt.get_optional<std::string>("Main." + process + "." + "configure");
        if(configuration)
          control->send_configuration(configuration.get());
        if (!process.compare("processor")) {
          std::string command_message = // send start_writing command to FrameProcessor
            "{\"id\":263,\"msg_type\":\"cmd\",\"msg_val\":\"execute\","
            "\"timestamp\":\"2024-11-21T08:53:06.340914\","
            "\"params\":{\"hdf\":{\"command\":\"start_writing\"}}}";
          control->send_configuration(command_message);
        }
      }
      else
        control->run_command();
      sleep(sleeptime);
    }

    for(int i=0; i<processes.size(); i++) {
      processes[i]->end();
    }

    for(int j=0; j<utilities.size(); j++) {
      int status = utilities[j]->exit_status();
      if (status != 0)
        return status;
    }

  } catch (const std::exception &e) {
    LOG4CXX_ERROR(logger, "Caught unhandled exception in FrameTestApp, application will terminate: " << e.what());
    throw;
  }

  return 0;

}
