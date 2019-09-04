/*
 * FrameProcessorApp.cpp
 *
 *  Created on: 25 May 2016
 *      Author: gnx91527
 */

#include <signal.h>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
using namespace std;

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/filesystem.hpp>
namespace po = boost::program_options;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

#include "logging.h"
#include "FrameProcessorController.h"
#include "DebugLevelLogger.h"
#include "SegFaultHandler.h"
#include "version.h"
#include "stringparse.h"

using namespace FrameProcessor;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void parse_arguments(int argc, char** argv, po::variables_map& vm, LoggerPtr& logger)
{
  try
  {
    std::string config_file;

    // Declare a group of options that will allowed only on the command line
    po::options_description generic("Generic options");
    generic.add_options()
        ("help,h",
         "Print this help message")
        ("version,v",
         "Print program version string")
        ;

    po::options_description config("Configuration options");
    config.add_options()
        ("debug-level,d",      po::value<unsigned int>()->default_value(debug_level),
           "Set the debug level")
        ("logconfig,l",   po::value<string>(),
           "Set the log4cxx logging configuration file")
        ("iothreads",     po::value<unsigned int>()->default_value(1),
           "Set number of IPC channel IO threads")
        ("ctrl",          po::value<std::string>()->default_value("tcp://0.0.0.0:5004"),
           "Set the control endpoint")
        ("ready",         po::value<std::string>()->default_value("tcp://127.0.0.1:5001"),
           "Ready ZMQ endpoint from frameReceiver")
        ("release",       po::value<std::string>()->default_value("tcp://127.0.0.1:5002"),
           "Release frame ZMQ endpoint from frameReceiver")
        ("meta",          po::value<std::string>()->default_value("tcp://*:5558"),
           "ZMQ meta data channel publish stream")
        ("json_file,j",  po::value<std::string>()->default_value(""),
         "Path to a JSON configuration file to submit to the application")
    ;

    // Parse the command line options
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    // If the command-line help option was given, print help and exit
    if (vm.count("help"))
    {
      std::cout << "Usage: frameProcessor [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      exit(1);
    }

    // If the command line version option was given, print version and exit
    if (vm.count("version")) {
      std::cout << "frameProcessor version " << ODIN_DATA_VERSION_STR << std::endl;
      exit(1);
    }

    if (vm.count("logconfig"))
    {
      std::string logconf_fname = vm["logconfig"].as<string>();
      if (has_suffix(logconf_fname, ".xml")) {
        log4cxx::xml::DOMConfigurator::configure(logconf_fname);
      } else {
        PropertyConfigurator::configure(logconf_fname);
      }
      LOG4CXX_DEBUG(logger, "log4cxx config file is set to " << vm["logconfig"].as<string>());
    } else {
      BasicConfigurator::configure();
    }

    if (vm.count("debug-level"))
    {
      set_debug_level(vm["debug-level"].as<unsigned int>());
      LOG4CXX_DEBUG_LEVEL(1, logger, "Debug level set to  " << debug_level);
    }

    if (vm.count("iothreads"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger, "Setting number of IO threads to " << vm["iothreads"].as<unsigned int>());
    }

  }
  catch (po::unknown_option &e)
  {
    LOG4CXX_WARN(logger, "CLI parsing error: " << e.what() << ". Will carry on...");
  }
  catch (Exception &e)
  {
    LOG4CXX_FATAL(logger, "Got Log4CXX exception: " << e.what());
    throw;
  }
  catch (exception &e)
  {
    LOG4CXX_ERROR(logger, "Got exception:" << e.what());
    throw;
  }
  catch (...)
  {
    LOG4CXX_FATAL(logger, "Exception of unknown type!");
    throw;
  }
}

int main(int argc, char** argv)
{
  // Initialise unexpected fault handling
  OdinData::init_seg_fault_handler();

  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LoggerPtr logger(Logger::getLogger("FP.App"));

  try {

    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    LOG4CXX_INFO(logger, "frameProcessor version " << ODIN_DATA_VERSION_STR << " starting up");

    unsigned int num_io_threads = vm["iothreads"].as<unsigned int>();
    boost::shared_ptr<FrameProcessorController> fwc;
    fwc = boost::shared_ptr<FrameProcessorController>(new FrameProcessorController(num_io_threads));

    // Configure the control channel for the FrameProcessor
    OdinData::IpcMessage cfg;
    OdinData::IpcMessage reply;
    cfg.set_param<std::string>("ctrl_endpoint", vm["ctrl"].as<string>());
    fwc->configure(cfg, reply);

    if (vm["json_file"].as<std::string>() != "") {
      std::string json_config_file = vm["json_file"].as<std::string>();
      // Attempt to open the file specified and read in the string as a JSON parameter set
      std::ifstream t(json_config_file.c_str());
      std::string json((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());

      // Check for empty JSON and throw an exception.
      if (json == "") {
        throw OdinData::OdinDataException("Incorrect or empty JSON configuration file specified");
      }

      // Parse the JSON file
      rapidjson::Document param_doc;

      if (param_doc.Parse(json.c_str()).HasParseError()) {
        std::stringstream msg;
        std::string error_snippet = extract_substr_at_pos(json, param_doc.GetErrorOffset(), 15);
        msg << "Parsing JSON configuration failed at line "
            << extract_line_no(json, param_doc.GetErrorOffset()) << ": "
            << rapidjson::GetParseError_En(param_doc.GetParseError()) << " " << error_snippet;
        throw OdinData::OdinDataException(msg.str());
      }

      // Check if the top level object is an array
      if (param_doc.IsArray()) {
        // Loop over the array submitting the child objects in order
        for (rapidjson::SizeType i = 0; i < param_doc.Size(); ++i) {
          // Create a configuration message
          OdinData::IpcMessage json_config_msg(param_doc[i],
                                               OdinData::IpcMessage::MsgTypeCmd,
                                               OdinData::IpcMessage::MsgValCmdConfigure);
          // Now submit the config to the controller
          fwc->configure(json_config_msg, reply);
        }
      } else {
        // Single level JSON object
        // Create a configuration message
        OdinData::IpcMessage json_config_msg(param_doc,
                                             OdinData::IpcMessage::MsgTypeCmd,
                                             OdinData::IpcMessage::MsgValCmdConfigure);
        // Now submit the config to the controller
        fwc->configure(json_config_msg, reply);
      }
    }

    fwc->run();

    LOG4CXX_DEBUG_LEVEL(1, logger, "FrameProcessorController run finished. Stopping app.");

  } catch (const std::exception& e) {
    LOG4CXX_ERROR(logger, "Caught unhandled exception in FrameProcessor, application will terminate: " << e.what());
    throw;
  }
  return 0;
}
