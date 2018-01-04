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
        ("config,c",    po::value<string>(&config_file),
         "Specify program configuration file")
        ;

    // Declare a group of options that will be allowed both on the command line
    // and in the configuration file
    po::options_description config("Configuration options");
    config.add_options()
        ("logconfig,l",   po::value<string>(),
           "Set the log4cxx logging configuration file")
        ("ctrl",          po::value<std::string>()->default_value("tcp://127.0.0.1:5004"),
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
        ("detector,d",    po::value<std::string>(),
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
           "Compression type of input data (0: None, 1: LZ4, 2: BSLZ4)")
        ("output,o",      po::value<std::string>()->default_value("test.hdf5"),
           "Name of HDF5 file to write frames to (default: test.hdf5)")
        ("output-dir",    po::value<std::string>()->default_value("/tmp/"),
           "Directory to write HDF5 file to (default: /tmp/)")
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
    ;

    // Group the variables for parsing at the command line and/or from the configuration file
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config);
    po::options_description config_file_options;
    config_file_options.add(config);

    // Parse the command line options
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    // If the command-line help option was given, print help and exit
    if (vm.count("help"))
    {
      std::cout << "usage: fileWriter [options]" << std::endl << std::endl;
      std::cout << cmdline_options << std::endl;
      exit(1);
    }

    // If the command line config option was given, parse the specified configuration
    // file for additional options. Note that boost::program_options gives precedence
    // to the first instance occurring. In this case, that implies that command-line
    // options have precedence over equivalent configuration file entries.
    if (vm.count("config"))
    {
      std::ifstream config_ifs(config_file.c_str());
      if (config_ifs)
      {
        LOG4CXX_DEBUG(logger, "Parsing configuration file " << config_file);
        po::store(po::parse_config_file(config_ifs, config_file_options, true), vm);
        po::notify(vm);
      }
      else
      {
        LOG4CXX_ERROR(logger, "Unable to open configuration file " << config_file << " for parsing");
        exit(1);
      }
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

    bool no_client = vm["no-client"].as<bool>();

    if (no_client)
    {
      LOG4CXX_DEBUG(logger, "Running FileWriter without client");
    }

    if (no_client && vm.count("ready"))
    {
      LOG4CXX_DEBUG(logger, "Setting frame ready notification ZMQ address to " << vm["ready"].as<string>());
    }

    if (no_client && vm.count("release"))
    {
      LOG4CXX_DEBUG(logger, "Setting frame release notification ZMQ address to " << vm["release"].as<string>());
    }

    if (no_client && vm.count("frames"))
    {
      LOG4CXX_DEBUG(logger, "Setting number of frames to receive to " << vm["frames"].as<unsigned int>());
    }

    if (no_client && vm.count("datasets"))
    {
      string message = "Configuring datasets: ";
      std::vector<string> datasets = vm["datasets"].as<std::vector<string> >();
      for (std::vector<string>::iterator it = datasets.begin(); it != datasets.end(); ++it) {
        message += *it + ", ";
      }
      LOG4CXX_DEBUG(logger, message);
    }

    if (no_client && vm.count("dims"))
    {
      string message = "Setting dataset dimensions to: ";
      std::vector<int> dims = vm["dims"].as<std::vector<int> >();
      for (std::vector<int>::iterator it = dims.begin(); it != dims.end(); ++it) {
        message += boost::lexical_cast<std::string>(*it) + ", ";
      }
      LOG4CXX_DEBUG(logger, message);
    }

    if (no_client && vm.count("chunk-dims"))
    {
      string message = "Setting dataset chunk dimensions to: ";
      std::vector<int> chunkDims = vm["chunk-dims"].as<std::vector<int> >();
      for (std::vector<int>::iterator it = chunkDims.begin(); it != chunkDims.end(); ++it) {
        message += boost::lexical_cast<std::string>(*it) + ", ";
      }
      LOG4CXX_DEBUG(logger, message);
    }

    if (no_client && vm.count("detector"))
    {
      LOG4CXX_DEBUG(logger, "Configuring for " << vm["detector"].as<string>() << " detector");
    }

    if (no_client && vm.count("bit-depth"))
    {
      LOG4CXX_DEBUG(logger, "Setting bit-depth to " << vm["bit-depth"].as<string>());
    }

    if (no_client && vm.count("dtype"))
    {
      LOG4CXX_DEBUG(logger, "Setting data type to " << vm["dtype"].as<int>());
    }

    if (no_client && vm.count("path"))
    {
      LOG4CXX_DEBUG(logger, "Setting shared library path to " << vm["path"].as<string>());
    }

    if (no_client && vm.count("compression"))
    {
      LOG4CXX_DEBUG(logger, "Setting compression type to " << vm["compression"].as<int>());
    }

    if (no_client && vm.count("ctrl"))
    {
      LOG4CXX_DEBUG(logger, "Setting control endpoint to: " << vm["ctrl"].as<string>());
    }

    if (no_client && vm.count("meta"))
    {
      LOG4CXX_DEBUG(logger, "Setting meta endpoint to: " << vm["meta"].as<string>());
    }

    if (no_client && vm.count("output"))
    {
      LOG4CXX_DEBUG(logger, "Writing frames to file: " << vm["output"].as<string>());
    }

    if (no_client && vm.count("processes"))
    {
      LOG4CXX_DEBUG(logger, "Number of concurrent file writer processes: " << vm["processes"].as<unsigned int>());
    }

    if (no_client && vm.count("rank"))
    {
      LOG4CXX_DEBUG(logger, "This process rank (index): " << vm["rank"].as<unsigned int>());
    }

    if (no_client && vm.count("acqid"))
    {
      LOG4CXX_DEBUG(logger, "Setting acquisition ID to " << vm["acqid"].as<string>());
    }

    if (no_client && vm.count("timeout"))
    {
      LOG4CXX_DEBUG(logger, "Setting close file timeout period to " << vm["timeout"].as<size_t>());
    }

    if (no_client && vm.count("block-size"))
    {
      LOG4CXX_DEBUG(logger, "Setting number of frames per block to " << vm["block-size"].as<size_t>());
    }

    if (no_client && vm.count("blocks-per-file"))
    {
      LOG4CXX_DEBUG(logger, "Setting number of blocks per file to " << vm["blocks-per-file"].as<size_t>());
    }

    if (no_client && vm["earliest-hdf-ver"].as<bool>())
    {
      LOG4CXX_DEBUG(logger, "Using earliest HDF5 version");
    }

    if (no_client && vm.count("alignment-threshold"))
    {
      LOG4CXX_DEBUG(logger, "Setting alignment threshold to " << vm["alignment-threshold"].as<size_t>());
    }

    if (no_client && vm.count("alignment-value"))
    {
      LOG4CXX_DEBUG(logger, "Setting alignment value to " << vm["alignment-value"].as<size_t>());
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

void configureController(boost::shared_ptr<FrameProcessorController> fwc,
                         po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  // Configure ZMQ channels
  cfg.set_param<string>("fr_setup/fr_ready_cnxn", vm["ready"].as<string>());
  cfg.set_param<string>("fr_setup/fr_release_cnxn", vm["release"].as<string>());
  cfg.set_param<string>("meta_endpoint", vm["meta"].as<string>());

  // Configure single-shot mode
  cfg.set_param<bool>("single-shot", vm["single-shot"].as<bool>());
  cfg.set_param<unsigned int>("frames", vm["frames"].as<unsigned int>());

  fwc->configure(cfg, reply);
}

void configureDetector(boost::shared_ptr<FrameProcessorController> fwc, po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  string name = vm["detector"].as<string>();
  name[0] = std::toupper(name[0]);
  string libraryName = name + "ProcessPlugin";
  boost::filesystem::path libraryPath = boost::filesystem::path(vm["path"].as<string>()) /
      boost::filesystem::path("lib" + libraryName + ".so");

  cfg.set_param<string>("plugin/load/library", libraryPath.string());
  cfg.set_param<string>("plugin/load/index", vm["detector"].as<string>());
  cfg.set_param<string>("plugin/load/name", libraryName);
  cfg.set_param<string>("plugin/connect/index", vm["detector"].as<string>());
  cfg.set_param<string>("plugin/connect/connection", "frame_receiver");
  if (vm.count("bit-depth")) {
    cfg.set_param<string>(vm["detector"].as<string>() + "/bitdepth", vm["bit-depth"].as<string>());
  }

  fwc->configure(cfg, reply);
}

void configureHDF5(boost::shared_ptr<FrameProcessorController> fwc, po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  cfg.set_param<string>("plugin/load/library", "./lib/libHdf5Plugin.so");
  cfg.set_param<string>("plugin/load/index", "hdf");
  cfg.set_param<string>("plugin/load/name", "FileWriterPlugin");
  cfg.set_param<string>("plugin/connect/index", "hdf");
  cfg.set_param<string>("plugin/connect/connection", vm["detector"].as<string>());
  cfg.set_param<unsigned int>("hdf/process/number", vm["processes"].as<unsigned int>());
  cfg.set_param<unsigned int>("hdf/process/rank", vm["rank"].as<unsigned int>());
  cfg.set_param<size_t>("hdf/process/frames_per_block", vm["block-size"].as<size_t>());
  cfg.set_param<size_t>("hdf/process/blocks_per_file", vm["blocks-per-file"].as<size_t>());
  cfg.set_param<bool>("hdf/process/earliest_version", vm["earliest-hdf-ver"].as<bool>());
  cfg.set_param<size_t>("hdf/process/alignment_threshold", vm["alignment-threshold"].as<size_t>());
  cfg.set_param<size_t>("hdf/process/alignment_value", vm["alignment-value"].as<size_t>());

  fwc->configure(cfg, reply);
}

void configureDataset(boost::shared_ptr<FrameProcessorController> fwc, po::variables_map vm,
                      string name, bool master=false) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  rapidjson::Document jsonDoc;
  rapidjson::Value jsonDims(rapidjson::kArrayType);
  rapidjson::Document::AllocatorType& dimAllocator = jsonDoc.GetAllocator();
  jsonDoc.SetObject();

  if (vm.count("dims")) {
    std::vector<int> dims = vm["dims"].as<std::vector<int> >();
    for (std::vector<int>::iterator it = dims.begin(); it != dims.end(); ++it) {
      jsonDims.PushBack(*it, dimAllocator);
    }
  }

  cfg.set_param<string>("hdf/dataset/cmd", "create");
  cfg.set_param<string>("hdf/dataset/name", name);
  cfg.set_param<int>("hdf/dataset/datatype", vm["dtype"].as<int>());
  cfg.set_param<rapidjson::Value>("hdf/dataset/dims", jsonDims);

  if (vm.count("chunk-dims")) {
    std::vector<int> chunkDims = vm["chunk-dims"].as<std::vector<int> >();
    rapidjson::Value jsonChunkDims(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType &chunkDimAllocator = jsonDoc.GetAllocator();
    jsonDoc.SetObject();
    for (std::vector<int>::iterator it2 = chunkDims.begin(); it2 != chunkDims.end(); ++it2) {
      jsonChunkDims.PushBack(*it2, chunkDimAllocator);
    }
    cfg.set_param<rapidjson::Value>("hdf/dataset/chunks", jsonChunkDims);
  }
  if (master) {
    cfg.set_param<string>("hdf/master", name);
  }
  if (vm.count("compression")) {
    cfg.set_param<int>("hdf/dataset/compression", vm["compression"].as<int>());
  }

  fwc->configure(cfg, reply);
}

void configureFileWriter(boost::shared_ptr<FrameProcessorController> fwc, po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;

  cfg.set_param<string>("hdf/file/name", vm["output"].as<string>());
  cfg.set_param<string>("hdf/file/path", vm["output-dir"].as<string>());
  cfg.set_param<unsigned int>("hdf/frames", vm["frames"].as<unsigned int>());
  cfg.set_param<string>("hdf/acquisition_id", vm["acqid"].as<string>());
  cfg.set_param<size_t>("hdf/timeout_timer_period", vm["timeout"].as<size_t>());
  cfg.set_param<bool>("hdf/write", true);

  fwc->configure(cfg, reply);
}

void configurePlugins(boost::shared_ptr<FrameProcessorController> fwc, po::variables_map vm) {
  configureDetector(fwc, vm);
  configureHDF5(fwc, vm);

  std::vector<string> datasets = vm["datasets"].as<std::vector<string> >();
  if (datasets.size() > 1) {
    configureDataset(fwc, vm, datasets[0], true);
    for (std::vector<string>::iterator it = datasets.begin() + 1; it != datasets.end(); ++it) {
      configureDataset(fwc, vm, *it, false);
    }
  }
  else {
    configureDataset(fwc, vm, datasets[0], false);
  }
}

void checkNoClientArgs(po::variables_map vm) {
  if (!(vm.count("detector") && vm.count("path") && vm.count("datasets")
        && vm.count("dtype") && vm.count("dims"))) {
    throw runtime_error("Must provide detector, path, datasets, dtype and "
                            "dims to run no client mode.");
  }
}

int main(int argc, char** argv)
{
  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LoggerPtr logger(Logger::getLogger("FP.App"));

  try {

    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    boost::shared_ptr<FrameProcessorController> fwc;
    fwc = boost::shared_ptr<FrameProcessorController>(new FrameProcessorController());

    // Configure the control channel for the FrameProcessor
    OdinData::IpcMessage cfg;
    OdinData::IpcMessage reply;
    cfg.set_param<std::string>("ctrl_endpoint", vm["ctrl"].as<string>());
    fwc->configure(cfg, reply);

    if (vm["init"].as<bool>() || vm["no-client"].as<bool>()) {
      configureController(fwc, vm);
      if (vm["no-client"].as<bool>()) {
        checkNoClientArgs(vm);
        LOG4CXX_DEBUG(logger, "Adding configuration options to work without a client");
        configurePlugins(fwc, vm);
        configureFileWriter(fwc, vm);
      }
    }

    fwc->run();

    LOG4CXX_DEBUG(logger, "FrameProcessorController run finished. Stopping app.");

  } catch (const std::exception& e) {
    LOG4CXX_ERROR(logger, e.what());
    // Nothing to do, terminate gracefully(?)
  }
  return 0;
}
