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
#include "FrameProcessorApp.h"
#include "FrameProcessorController.h"
#include "DebugLevelLogger.h"
#include "SegFaultHandler.h"
#include "version.h"
#include "stringparse.h"

using namespace FrameProcessor;

boost::shared_ptr<FrameProcessorController> FrameProcessorApp::controller_;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

FrameProcessorApp::FrameProcessorApp(void) : json_config_file_("")
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

int FrameProcessorApp::parse_arguments(int argc, char** argv)
{
  int rc = 0;
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
        ("config,c",    po::value<string>(&config_file),
         "Specify program configuration file")
        ;

    // Declare a group of options that will be allowed both on the command line
    // and in the configuration file
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
        ("init,I",        po::bool_switch(),
           "Initialise frame receiver and meta interfaces.")
        ("no-client,N",   po::bool_switch(),
           "Enable full initial configuration to run without any client controller."
           "You must also be provide: detector, path, datasets, dtype and dims.")
        ("processes,p",   po::value<unsigned int>()->default_value(1),
           "Number of concurrent file writer processes")
        ("rank,r",        po::value<unsigned int>()->default_value(0),
           "The rank (index number) of the current file writer process in relation to the other concurrent ones")
        ("detector",      po::value<std::string>(),
           "Detector to configure for")
        ("path",          po::value<std::string>(),
           "Path to detector shared library with format 'lib<detector>ProcessPlugin.so'")
        ("datasets",      po::value<std::vector<string> >()->multitoken(),
           "Name(s) of datasets to write (space separated)")
        ("dtype",         po::value<int>(),
           "Data type of raw detector data (0: 8bit, 1: 16bit, 2: 32bit, 3:64bit)")
        ("dims",          po::value<std::vector<int> >()->multitoken(),
           "Dimensions of each frame (space separated)")
        ("chunk-dims,C",  po::value<std::vector<int> >()->multitoken(),
           "Chunk size of each sub-frame (space separated)")
        ("bit-depth",     po::value<std::string>(),
           "Bit-depth mode of detector")
        ("compression",   po::value<int>(),
           "Compression type of input data (0: None, 1: LZ4, 2: BSLZ4, 3: Blosc)")
        ("output,o",      po::value<std::string>()->default_value("test.hdf5"),
           "Name of HDF5 file to write frames to (default: test.hdf5)")
        ("output-dir",    po::value<std::string>()->default_value("/tmp/"),
           "Directory to write HDF5 file to (default: /tmp/)")
        ("extension",     po::value<std::string>()->default_value("h5"),
           "Set the file extension of the data files (default: h5)")
        ("single-shot,S", po::bool_switch(),
           "Shutdown after one dataset completed")
        ("frames,f",      po::value<unsigned int>()->default_value(0),
           "Set the number of frames to write into dataset")
        ("acqid",         po::value<std::string>()->default_value(""),
           "Set the Acquisition Id of the acquisition")
        ("timeout",       po::value<size_t>()->default_value(0),
           "Set the timeout period for closing the file (milliseconds)")
        ("block-size",       po::value<size_t>()->default_value(1),
           "Set the number of consecutive frames to write per block")
        ("blocks-per-file",       po::value<size_t>()->default_value(0),
           "Set the number of blocks to write to file. Default is 0 (unlimited)")
        ("earliest-hdf-ver",      po::bool_switch(),
           "Set to use earliest hdf5 file version. Default is off (use latest)")
        ("alignment-threshold",       po::value<size_t>()->default_value(1),
           "Set the hdf5 alignment threshold. Default is 1 (no alignment)")
        ("alignment-value",       po::value<size_t>()->default_value(1),
           "Set the hdf5 alignment value. Default is 1 (no alignment)")
        ("json_file,j",  po::value<std::string>()->default_value(""),
         "Path to a JSON configuration file to submit to the application")
    ;

    // Group the variables for parsing at the command line and/or from the configuration file
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);
    po::options_description config_file_options;
    config_file_options.add(config);

    // Parse the command line options
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm_);
    po::notify(vm_);

    // If the command-line help option was given, print help and exit
    if (vm_.count("help"))
    {
      std::cout << "usage: frameProcessor [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      return 1;
    }

    // If the command line version option was given, print version and exit
    if (vm_.count("version")) {
      std::cout << "frameProcessor version " << ODIN_DATA_VERSION_STR << std::endl;
      return 1;
    }

    // If the command line config option was given, parse the specified configuration
    // file for additional options. Note that boost::program_options gives precedence
    // to the first instance occurring. In this case, that implies that command-line
    // options have precedence over equivalent configuration file entries.
    if (vm_.count("config"))
    {
      std::ifstream config_ifs(config_file.c_str());
      if (config_ifs)
      {
        LOG4CXX_DEBUG(logger_, "Parsing configuration file " << config_file);
        po::store(po::parse_config_file(config_ifs, config_file_options, true), vm_);
        po::notify(vm_);
      }
      else
      {
        LOG4CXX_ERROR(logger_, "Unable to open configuration file " << config_file << " for parsing");
        return 1;
      }
    }

    if (vm_.count("logconfig"))
    {
      std::string logconf_fname = vm_["logconfig"].as<string>();
      if (has_suffix(logconf_fname, ".xml")) {
        log4cxx::xml::DOMConfigurator::configure(logconf_fname);
      } else {
        PropertyConfigurator::configure(logconf_fname);
      }
      LOG4CXX_DEBUG(logger_, "log4cxx config file is set to " << vm_["logconfig"].as<string>());
    }

    if (vm_.count("debug-level"))
    {
      set_debug_level(vm_["debug-level"].as<unsigned int>());
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug_level);
    }

    if (vm_.count("iothreads"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of IO threads to " << vm_["iothreads"].as<unsigned int>());
    }

    bool no_client = vm_["no-client"].as<bool>();

    if (no_client)
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Running FileWriter without client");
    }

    if (no_client && vm_.count("ready"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting frame ready notification ZMQ address to " << vm_["ready"].as<string>());
    }

    if (no_client && vm_.count("release"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting frame release notification ZMQ address to " << vm_["release"].as<string>());
    }

    if (no_client && vm_.count("frames"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of frames to receive to " << vm_["frames"].as<unsigned int>());
    }

    if (no_client && vm_.count("datasets"))
    {
      string message = "Configuring datasets: ";
      std::vector<string> datasets = vm_["datasets"].as<std::vector<string> >();
      for (std::vector<string>::iterator it = datasets.begin(); it != datasets.end(); ++it) {
        message += *it + ", ";
      }
      LOG4CXX_DEBUG_LEVEL(1, logger_, message);
    }

    if (no_client && vm_.count("dims"))
    {
      string message = "Setting dataset dimensions to: ";
      std::vector<int> dims = vm_["dims"].as<std::vector<int> >();
      for (std::vector<int>::iterator it = dims.begin(); it != dims.end(); ++it) {
        message += boost::lexical_cast<std::string>(*it) + ", ";
      }
      LOG4CXX_DEBUG_LEVEL(1, logger_, message);
    }

    if (no_client && vm_.count("chunk-dims"))
    {
      string message = "Setting dataset chunk dimensions to: ";
      std::vector<int> chunkDims = vm_["chunk-dims"].as<std::vector<int> >();
      for (std::vector<int>::iterator it = chunkDims.begin(); it != chunkDims.end(); ++it) {
        message += boost::lexical_cast<std::string>(*it) + ", ";
      }
      LOG4CXX_DEBUG_LEVEL(1, logger_, message);
    }

    if (no_client && vm_.count("detector"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Configuring for " << vm_["detector"].as<string>() << " detector");
    }

    if (no_client && vm_.count("bit-depth"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting bit-depth to " << vm_["bit-depth"].as<string>());
    }

    if (no_client && vm_.count("dtype"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting data type to " << vm_["dtype"].as<int>());
    }

    if (no_client && vm_.count("path"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting shared library path to " << vm_["path"].as<string>());
    }

    if (no_client && vm_.count("compression"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting compression type to " << vm_["compression"].as<int>());
    }

    if (no_client && vm_.count("ctrl"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting control endpoint to: " << vm_["ctrl"].as<string>());
    }

    if (no_client && vm_.count("meta"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting meta endpoint to: " << vm_["meta"].as<string>());
    }

    if (no_client && vm_.count("output"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Writing frames to file: " << vm_["output"].as<string>());
    }

    if (no_client && vm_.count("processes"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Number of concurrent file writer processes: " << vm_["processes"].as<unsigned int>());
    }

    if (no_client && vm_.count("rank"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "This process rank (index): " << vm_["rank"].as<unsigned int>());
    }

    if (no_client && vm_.count("acqid"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting acquisition ID to " << vm_["acqid"].as<string>());
    }

    if (no_client && vm_.count("timeout"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting close file timeout period to " << vm_["timeout"].as<size_t>());
    }

    if (no_client && vm_.count("block-size"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of frames per block to " << vm_["block-size"].as<size_t>());
    }

    if (no_client && vm_.count("blocks-per-file"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of blocks per file to " << vm_["blocks-per-file"].as<size_t>());
    }

    if (no_client && vm_["earliest-hdf-ver"].as<bool>())
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Using earliest HDF5 version");
    }

    if (no_client && vm_.count("alignment-threshold"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting alignment threshold to " << vm_["alignment-threshold"].as<size_t>());
    }

    if (no_client && vm_.count("alignment-value"))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting alignment value to " << vm_["alignment-value"].as<size_t>());
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

bool FrameProcessorApp::isBloscRequired()
{
  return vm_.count("compression") && vm_["compression"].as<int>() == 3;
}

void FrameProcessorApp::configureController()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  // Configure ZMQ channels
  cfg.set_param<string>("fr_setup/fr_ready_cnxn", vm_["ready"].as<string>());
  cfg.set_param<string>("fr_setup/fr_release_cnxn", vm_["release"].as<string>());
  cfg.set_param<string>("meta_endpoint", vm_["meta"].as<string>());

  // Configure single-shot mode
  cfg.set_param<bool>("single-shot", vm_["single-shot"].as<bool>());
  cfg.set_param<unsigned int>("frames", vm_["frames"].as<unsigned int>());

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureDetector()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  string name = vm_["detector"].as<string>();
  name[0] = std::toupper(name[0]);
  string libraryName = name + "ProcessPlugin";
  boost::filesystem::path libraryPath = boost::filesystem::path(vm_["path"].as<string>()) /
      boost::filesystem::path("lib" + libraryName + ".so");

  cfg.set_param<string>("plugin/load/library", libraryPath.string());
  cfg.set_param<string>("plugin/load/index", vm_["detector"].as<string>());
  cfg.set_param<string>("plugin/load/name", libraryName);
  cfg.set_param<string>("plugin/connect/index", vm_["detector"].as<string>());
  cfg.set_param<string>("plugin/connect/connection", "frame_receiver");
  if (vm_.count("bit-depth")) {
    cfg.set_param<string>(vm_["detector"].as<string>() + "/bitdepth", vm_["bit-depth"].as<string>());
  }

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureDetectorDecoder()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  string detector_decoder = vm_["detector"].as<string>();
  detector_decoder[0] = std::tolower(detector_decoder[0]);

  if (vm_.count("bit-depth")) {
    cfg.set_param<string>(detector_decoder + "/bitdepth", vm_["bit-depth"].as<string>());
  }

  if (vm_.count("dims")) {
    std::vector<int> dims = vm_["dims"].as<std::vector<int> >();
    cfg.set_param<int>(detector_decoder + "/height", dims[0]);
    cfg.set_param<int>(detector_decoder + "/width", dims[1]);
  }
  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureBlosc()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  cfg.set_param<string>("plugin/load/library", "./lib/libBloscPlugin.so");
  cfg.set_param<string>("plugin/load/index", "blosc");
  cfg.set_param<string>("plugin/load/name", "BloscPlugin");
  cfg.set_param<string>("plugin/connect/index", "blosc");
  cfg.set_param<string>("plugin/connect/connection", vm_["detector"].as<string>());

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureHDF5()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  cfg.set_param<string>("plugin/load/library", "./lib/libHdf5Plugin.so");
  cfg.set_param<string>("plugin/load/index", "hdf");
  cfg.set_param<string>("plugin/load/name", "FileWriterPlugin");
  cfg.set_param<string>("plugin/connect/index", "hdf");
  if (isBloscRequired()) {
    cfg.set_param<string>("plugin/connect/connection", "blosc");
  } else {
    cfg.set_param<string>("plugin/connect/connection", vm_["detector"].as<string>());
  }
  cfg.set_param<unsigned int>("hdf/process/number", vm_["processes"].as<unsigned int>());
  cfg.set_param<unsigned int>("hdf/process/rank", vm_["rank"].as<unsigned int>());
  cfg.set_param<size_t>("hdf/process/frames_per_block", vm_["block-size"].as<size_t>());
  cfg.set_param<size_t>("hdf/process/blocks_per_file", vm_["blocks-per-file"].as<size_t>());
  cfg.set_param<bool>("hdf/process/earliest_version", vm_["earliest-hdf-ver"].as<bool>());
  cfg.set_param<size_t>("hdf/process/alignment_threshold", vm_["alignment-threshold"].as<size_t>());
  cfg.set_param<size_t>("hdf/process/alignment_value", vm_["alignment-value"].as<size_t>());

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureDataset(string name, bool master)
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  rapidjson::Document jsonDoc;
  rapidjson::Value jsonDims(rapidjson::kArrayType);
  rapidjson::Document::AllocatorType& dimAllocator = jsonDoc.GetAllocator();
  jsonDoc.SetObject();

  if (vm_.count("dims")) {
    std::vector<int> dims = vm_["dims"].as<std::vector<int> >();
    for (std::vector<int>::iterator it = dims.begin(); it != dims.end(); ++it) {
      jsonDims.PushBack(*it, dimAllocator);
    }
  }

  cfg.set_param<int>("hdf/dataset/" + name + "/datatype", vm_["dtype"].as<int>());
  cfg.set_param<rapidjson::Value>("hdf/dataset/" + name + "/dims", jsonDims);

  if (vm_.count("chunk-dims")) {
    std::vector<int> chunkDims = vm_["chunk-dims"].as<std::vector<int> >();
    rapidjson::Value jsonChunkDims(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType &chunkDimAllocator = jsonDoc.GetAllocator();
    jsonDoc.SetObject();
    for (std::vector<int>::iterator it2 = chunkDims.begin(); it2 != chunkDims.end(); ++it2) {
      jsonChunkDims.PushBack(*it2, chunkDimAllocator);
    }
    cfg.set_param<rapidjson::Value>("hdf/dataset/" + name + "/chunks", jsonChunkDims);
  }
  if (master) {
    cfg.set_param<string>("hdf/master", name);
  }
  if (vm_.count("compression")) {
    cfg.set_param<int>("hdf/dataset/" + name + "/compression", vm_["compression"].as<int>());
  }

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configureFileWriter()
{
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  cfg.set_param<string>("hdf/file/name", vm_["output"].as<string>());
  cfg.set_param<string>("hdf/file/path", vm_["output-dir"].as<string>());
  cfg.set_param<string>("hdf/file/extension", vm_["extension"].as<string>());
  cfg.set_param<unsigned int>("hdf/frames", vm_["frames"].as<unsigned int>());
  cfg.set_param<string>("hdf/acquisition_id", vm_["acqid"].as<string>());
  cfg.set_param<size_t>("hdf/timeout_timer_period", vm_["timeout"].as<size_t>());
  cfg.set_param<bool>("hdf/write", true);

  controller_->configure(cfg, reply);
}

void FrameProcessorApp::configurePlugins()
{
  configureDetector();
  configureDetectorDecoder();
  if (isBloscRequired()) {
    configureBlosc();
  }
  configureHDF5();

  std::vector<string> datasets = vm_["datasets"].as<std::vector<string> >();
  if (datasets.size() > 1) {
    configureDataset(datasets[0], true);
    for (std::vector<string>::iterator it = datasets.begin() + 1; it != datasets.end(); ++it) {
      configureDataset(*it, false);
    }
  }
  else {
    configureDataset(datasets[0], false);
  }
}

void FrameProcessorApp::checkNoClientArgs() {
  if (!(vm_.count("detector") && vm_.count("path") && vm_.count("datasets")
        && vm_.count("dtype") && vm_.count("dims"))) {
    throw runtime_error("Must provide detector, path, datasets, dtype and "
                            "dims to run no client mode.");
  }
}

void FrameProcessorApp::run(void)
{

  LOG4CXX_INFO(logger_, "frameProcessor version " << ODIN_DATA_VERSION_STR << " starting up");

  try {

    // Instantiate a controller
    unsigned int num_io_threads = vm_["iothreads"].as<unsigned int>();
    controller_ = boost::shared_ptr<FrameProcessorController>(
        new FrameProcessorController(num_io_threads)
    );

    // Configure the control channel for the FrameProcessor
    OdinData::IpcMessage cfg;
    OdinData::IpcMessage reply;
    cfg.set_param<std::string>("ctrl_endpoint", vm_["ctrl"].as<string>());
    controller_->configure(cfg, reply);

    if (vm_["init"].as<bool>() || vm_["no-client"].as<bool>()) {
      configureController();
      if (vm_["no-client"].as<bool>()) {
        checkNoClientArgs();
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Adding configuration options to work without a client");
        configurePlugins();
        configureFileWriter();
      }
    }

    if (vm_["json_file"].as<std::string>() != "") {
      std::string json_config_file = vm_["json_file"].as<std::string>();
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
          controller_->configure(json_config_msg, reply);
        }
      } else {
        // Single level JSON object
        // Create a configuration message
        OdinData::IpcMessage json_config_msg(param_doc,
                                             OdinData::IpcMessage::MsgTypeCmd,
                                             OdinData::IpcMessage::MsgValCmdConfigure);
        // Now submit the config to the controller
        controller_->configure(json_config_msg, reply);
      }
    }

    controller_->run();

    LOG4CXX_DEBUG_LEVEL(1, logger_, "FrameProcessorController run finished. Stopping app.");

  }
  catch (const std::exception& e) {
    LOG4CXX_ERROR(logger_, "Caught unhandled exception in FrameProcessor, application will terminate: " << e.what());
    throw;
  }
}

int main (int argc, char** argv)
{
  int rc = 0;

  // Initialise unexpected fault handling
  OdinData::init_seg_fault_handler();

  // Set the locale and application path for logging
  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];

  // Create a FrameProcessorApp instance
  FrameProcessor::FrameProcessorApp fp_instance;

  // Parse commnd line arguments
  rc = fp_instance.parse_arguments (argc, argv);

  // Run the frame processor
  if (rc == 0)
  {
    fp_instance.run ();
  }

  return rc;
}