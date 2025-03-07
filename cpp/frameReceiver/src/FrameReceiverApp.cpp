/**
 * FrameReceiverApp.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <signal.h>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
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
namespace po = boost::program_options;

#include "logging.h"
#include "FrameReceiverApp.h"
#include "SegFaultHandler.h"
#include "version.h"
#include "stringparse.h"

using namespace FrameReceiver;

std::shared_ptr<FrameReceiverController> FrameReceiverApp::controller_;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

//! Constructor for FrameReceiverApp class.
//!
//! This constructor initialises the FrameReceiverApp instance

FrameReceiverApp::FrameReceiverApp(void) : config_file_("")
{

  // Retrieve a logger instance
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  BasicConfigurator::configure();
  logger_ = Logger::getLogger("FR.App");

}

//! Destructor for FrameReceiverApp class

FrameReceiverApp::~FrameReceiverApp()
{
  controller_.reset();
}

//! Parse command-line arguments and configuration file options.
//!
//! This method parses command-line arguments and configuration file options
//! to configure the application for operation. Most options can either be
//! given at the command line or stored in an INI-formatted configuration file.
//! The configuration options are stored in the FrameReceiverConfig helper object for
//! retrieval throughout the application.
//!
//! \param argc - standard command-line argument count
//! \param argv - array of char command-line options
//! \return return code:
//!   * -1 if option parsing succeeded and we should continue running the application
//!   *  0 if specific command completed (e.g. --help) and we should exit with success
//!   *  1 if option parsing failed and we should exit with failure

int FrameReceiverApp::parse_arguments(int argc, char** argv)
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

    // Declare a group of options that will be allowed both on the command line
    // and in the configuration file
    po::options_description config("Configuration options");
    config.add_options()
        ("debug-level,d", po::value<unsigned int>()->default_value(debug_level),
        "Set the debug level")
        ("log-config,l", po::value<string>(),
        "Set the log4cxx logging configuration file")
        ("io-threads", po::value<unsigned int>()->default_value(OdinData::Defaults::default_io_threads),
        "Set number of IPC channel IO threads")
        ("ctrl", po::value<std::string>()->default_value(FrameReceiver::Defaults::default_ctrl_chan_endpoint),
        "Set the control channel endpoint")
        ("config,c",  po::value<std::string>(),
         "Path to a JSON configuration file to submit to the application")
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
      std::cout << "usage: frameReceiver [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      return 0;
    }

    // If the command line version option was given, print version and exit
    if (vm.count("version"))
    {
      std::cout << "frameReceiver version " ODIN_DATA_VERSION_STR << std::endl;
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
      LOG4CXX_DEBUG_LEVEL(1, logger_, "log4cxx config file is set to " << log_config);
    }

    if (vm.count("debug-level"))
    {
      set_debug_level(vm["debug-level"].as<unsigned int>());
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug_level);
    }

    if (vm.count("io-threads"))
    {
      io_threads_ = vm["io-threads"].as<unsigned int>();
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of IO threads to " << io_threads_);
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
    LOG4CXX_ERROR(logger_, "Got Log4CXX exception: " << e.what());
    rc = 1;
  }
  catch (exception &e)
  {
    LOG4CXX_ERROR(logger_, "Got exception during command line argument parsing: " << e.what());
    rc = 1;
  }
  catch (...)
  {
    LOG4CXX_FATAL(logger_, "Got unknown exception during command line argument parsing");
    throw;
  }

  return rc;

}

//! Run the FrameReceiverApp.
//!
//! This method runs the frame receiver. The controller is initialised from
//! the input configuration parameters and then run, blocking until completion
//! of execution.

int FrameReceiverApp::run(void)
{

  int rc = 0;

  LOG4CXX_INFO(logger_, "frameReceiver version " << ODIN_DATA_VERSION_STR << " starting up");

  // Instantiate a controller
  controller_ = std::shared_ptr<FrameReceiverController>(new FrameReceiverController(io_threads_));

  try {

    // Send an initial configuration message to the controller to configure control and RX thread
    // channel endpoints
    OdinData::IpcMessage config_msg;
    OdinData::IpcMessage config_reply;
    config_msg.set_param<std::string>(CONFIG_CTRL_ENDPOINT, ctrl_channel_endpoint_);
    config_msg.set_param<std::string>(CONFIG_RX_ENDPOINT, Defaults::default_rx_chan_endpoint);
    controller_->configure(config_msg, config_reply);

    // Check for a JSON configuration file option
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
      if (config_json.IsArray()) {
        // Loop over the array submitting the child objects in order
        for (rapidjson::SizeType i = 0; i < config_json.Size(); ++i) {
          // Create a configuration message
          OdinData::IpcMessage json_config_msg(
            config_json[i], OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdConfigure
          );
          // Now submit the config to the controller
          controller_->configure(json_config_msg, config_reply);
        }
      } else {
        // Single level JSON object
        // Create a configuration message
        OdinData::IpcMessage json_config_msg(
          config_json, OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdConfigure
        );
        // Now submit the config to the controller
        controller_->configure(json_config_msg, config_reply);
      }
    }

    controller_->run();

    LOG4CXX_INFO(logger_, "frameReceiver stopped");

  }
  catch (OdinData::OdinDataException& e)
  {
    LOG4CXX_ERROR(logger_, "frameReceiver run failed: " << e.what());
    rc = 1;
  }
  catch (exception& e)
  {
    LOG4CXX_ERROR(logger_, "Generic exception during frameReceiver run:\n" << e.what());
    rc = 1;
  }
  catch (...)
  {
    LOG4CXX_ERROR(logger_, "Unexpected exception during frameReceiver run");
    rc = 1;
  }

  return rc;
}

//! Stop the frameRecevierApp.
//!
//! This method stops the frame receiver by signalling to the controller to stop.

void FrameReceiverApp::stop(void)
{
  controller_->stop();
}

//! Interrupt signal handler

void intHandler (int sig)
{
  FrameReceiver::FrameReceiverApp::stop();
}

//! Main application entry point

int main (int argc, char** argv)
{
  int rc = -1;

  // Initialise unexpected fault handling
  OdinData::init_seg_fault_handler();

  // Trap Ctrl-C and pass to interrupt handler
  signal (SIGINT, intHandler);
  signal (SIGTERM, intHandler);

  // Set the application path and locale for logging
  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];

  // Create a FrameReceiverApp instance
  FrameReceiver::FrameReceiverApp app;

  // Parse command line arguments and set up node configuration
  rc = app.parse_arguments(argc, argv);

  if (rc == -1) {
    // Run the application
    rc = app.run();
  }

  return rc;
}
