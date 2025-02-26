/*
 * FrameProcessorApp.cpp
 *
 *  Created on: 25 May 2016
 *      Author: Alan Greer
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

// #include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
namespace po = boost::program_options;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

#include "logging.h"
#include "FrameProcessorApp.h"
#include "FrameProcessorController.h"
#include "DebugLevelLogger.h"
#include "SegFaultHandler.h"
#include "version.h"
#include "stringparse.h"

using namespace FrameProcessor;

std::shared_ptr<FrameProcessorController> FrameProcessorApp::controller_;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

FrameProcessorApp::FrameProcessorApp(void) : config_file_("")
{
  // Retrieve and configure a logger instance
    OdinData::configure_logging_mdc(OdinData::app_path.c_str());
    BasicConfigurator::configure();
    logger_ = Logger::getLogger("FP.App");
}

FrameProcessorApp::~FrameProcessorApp()
{
  controller_.reset();
}

/** Parse command line arguments
 *
 * @return return code:
 *   * -1 if option parsing succeeded and we should continue running the application
 *   *  0 if specific command completed (e.g. --help) and we should exit with success
 *   *  1 if option parsing failed and we should exit with failure
 */
int FrameProcessorApp::parse_arguments(int argc, char** argv)
{
  int rc = -1;
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
        ("debug-level,d", po::value<unsigned int>()->default_value(debug_level), "Set the debug level")
        ("log-config,l", po::value<string>(), "Set the log4cxx logging configuration file")
        ("io-threads", po::value<unsigned int>()->default_value(OdinData::Defaults::default_io_threads), "Set number of IPC channel IO threads")
        ("ctrl", po::value<std::string>()->default_value("tcp://127.0.0.1:5004"), "Set the control endpoint")
        ("config,c", po::value<std::string>()->default_value(""), "File path of inital JSON config for controller")
    ;

    // Group the variables for parsing at the command line
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);

    // Parse the command line options
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    // If the command-line help option was given, print help and exit
    if (vm.count("help"))
    {
      std::cout << "Usage: frameProcessor [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      return 0;
    }

    // If the command line version option was given, print version and exit
    if (vm.count("version")) {
      std::cout << "frameProcessor version " << ODIN_DATA_VERSION_STR << std::endl;
      return 0;
    }

    if (vm.count("log-config"))
    {
      std::string log_config = vm["log-config"].as<string>();
      if (has_suffix(log_config, ".xml")) {
        log4cxx::xml::DOMConfigurator::configure(log_config);
      } else {
        PropertyConfigurator::configure(log_config);
      }
      LOG4CXX_DEBUG(logger_, "log4cxx config file is set to " << vm["log-config"].as<string>());
    }

    if (vm.count("debug-level"))
    {
      set_debug_level(vm["debug-level"].as<unsigned int>());
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug_level);
    }

    if (vm.count("io-threads"))
    {
      io_threads_ = vm["io-threads"].as<unsigned int>();
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of IO threads to " << vm["io-threads"].as<unsigned int>());
    }

    if (vm.count("ctrl"))
    {
      ctrl_channel_endpoint_ = vm["ctrl"].as<std::string>();
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting control channel endpoint to " << ctrl_channel_endpoint_);
    }

    if (vm.count("config"))
    {
      config_file_ = vm["config"].as<std::string>();
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Loading JSON configuration file " << config_file_);
    }

  }
  catch (po::unknown_option &e)
  {
    LOG4CXX_ERROR(logger_, "Error parsing command line arguments: " << e.what());
    rc = 1;
  }
  catch (Exception &e)
  {
    LOG4CXX_FATAL(logger_, "Got Log4CXX exception: " << e.what());
    rc = 1;
  }
  catch (exception &e)
  {
    LOG4CXX_ERROR(logger_, "Got exception during command line argument parsing: " << e.what());
    rc = 1;
  }
  catch (...)
  {
    LOG4CXX_FATAL(logger_, "Exception of unknown type!");
    throw;
  }

  return rc;
}

int FrameProcessorApp::run(void)
{

  int rc = 0;

  LOG4CXX_INFO(logger_, "frameProcessor version " << ODIN_DATA_VERSION_STR << " starting up");

    // Instantiate a controller
    controller_ = std::shared_ptr<FrameProcessorController>(
      new FrameProcessorController(io_threads_)
    );

  try {

    // Configure the control channel for the FrameProcessor
    OdinData::IpcMessage ctrl_endpoint_cfg;
    OdinData::IpcMessage reply;
    ctrl_endpoint_cfg.set_param<std::string>("ctrl_endpoint", ctrl_channel_endpoint_);
    controller_->configure(ctrl_endpoint_cfg, reply);

    if (config_file_ != "") {
      // Attempt to open the file specified and read in the string as a JSON parameter set
      std::ifstream config_file_stream(config_file_.c_str());
      std::string config_text((std::istreambuf_iterator<char>(config_file_stream)), std::istreambuf_iterator<char>());

      // Check for empty JSON and throw an exception.
      if (config_text == "") {
        throw OdinData::OdinDataException("Incorrect or empty JSON configuration file specified");
      }

      // Parse the JSON file
      rapidjson::Document config_json;

      if (config_json.Parse(config_text.c_str()).HasParseError()) {
        std::stringstream msg;
        std::string error_snippet = extract_substr_at_pos(config_text, config_json.GetErrorOffset(), 15);
        msg << "Parsing JSON configuration failed at line "
          << extract_line_no(config_text, config_json.GetErrorOffset()) << ": "
          << rapidjson::GetParseError_En(config_json.GetParseError()) << " " << error_snippet;
        throw OdinData::OdinDataException(msg.str());
      }

      // Check if the top level object is an array
      OdinData::IpcMessage config_msg;
      if (config_json.IsArray()) {
        // Loop over the array submitting the child objects in order
        for (rapidjson::SizeType i = 0; i < config_json.Size(); ++i) {
          OdinData::IpcMessage config_msg(
            config_json[i], OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdConfigure
          );
          configure_controller(config_msg);
        }
      } else {
        // Single level JSON object
        OdinData::IpcMessage json_config_msg(
          config_json, OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdConfigure
        );
        configure_controller(config_msg);
      }
    }

    controller_->run();

    LOG4CXX_DEBUG_LEVEL(1, logger_, "frameProcessor stopped");

  }
  catch (OdinData::OdinDataException& e)
  {
    LOG4CXX_ERROR(logger_, "frameProcessor run failed: " << e.what());
    rc = 1;
  }
  catch (const std::exception& e) {
    LOG4CXX_ERROR(logger_, "Caught unhandled exception in frameProcessor, application will terminate: " << e.what());
    rc = 1;
  }

  return rc;
}

/**
 * Configure the controller with the given configuration message.
 *
 * Any runtime_error from the controller is caught and reported as an error.
 *
 * @param config_msg the message containing the configuration
 */
void FrameProcessorApp::configure_controller(OdinData::IpcMessage& config_msg)
{
  OdinData::IpcMessage reply;
  try {
    controller_->configure(config_msg, reply);
  } catch (const std::runtime_error& e) {
    LOG4CXX_ERROR(logger_, "Failed to configure controller with message " << config_msg.encode());
  }
}

int main (int argc, char** argv)
{
  int rc = -1;

  // Initialise unexpected fault handling
  OdinData::init_seg_fault_handler();

  // Set the locale and application path for logging
  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];

  // Create a FrameProcessorApp instance
  FrameProcessor::FrameProcessorApp app;

  // Parse commnd line arguments
  rc = app.parse_arguments(argc, argv);

  if (rc == -1) {
    // Run the application
    rc = app.run();
  }

  return rc;
}
