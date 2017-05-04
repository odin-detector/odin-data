/*
 * testSocket.cpp
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
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
namespace po = boost::program_options;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

#include "zmq/zmq.hpp"

#include "FileWriterController.h"
#include "IpcMessage.h"
#include "SharedMemoryController.h"

using namespace filewriter;

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
                ("logconfig,l",  po::value<string>(),
                    "Set the log4cxx logging configuration file")
                ("no-client,N",  po::bool_switch(),
                    "Enable full initial configuration to run without any client controller")
                ("single-shot,S",  po::bool_switch(),
                    "Shutdown after one dataset written")
                ("ready",       po::value<std::string>()->default_value("tcp://127.0.0.1:5001"),
                    "Ready ZMQ endpoint from frameReceiver")
                ("release",       po::value<std::string>()->default_value("tcp://127.0.0.1:5002"),
                        "Release frame ZMQ endpoint from frameReceiver")
                ("frames,f",     po::value<unsigned int>()->default_value(0),
                    "Set the number of frames to be notified about before terminating")
                ("detector,d",   po::value<std::string>()->default_value("excalibur"),
                    "Detector type to confugre for")
                ("sharedbuf",    po::value<std::string>()->default_value("FrameReceiverBuffer"),
                    "Set the control endpoint")
                ("ctrl",    po::value<std::string>()->default_value("tcp://127.0.0.1:5004"),
                    "Set the name of the shared memory frame buffer")
                ("output,o",     po::value<std::string>()->default_value("test.hdf5"),
                    "Name of HDF5 file to write frames to (default: test.hdf5)")
                ("processes,p",  po::value<unsigned int>()->default_value(1),
                    "Number of concurrent file writer processes"   )
                ("rank,r",       po::value<unsigned int>()->default_value(0),
                    "The rank (index number) of the current file writer process in relation to the other concurrent ones")
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
            PropertyConfigurator::configure(vm["logconfig"].as<string>());
            LOG4CXX_DEBUG(logger, "log4cxx config file is set to " << vm["logconfig"].as<string>());
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
        
        if (no_client && vm.count("detector"))
        {
          LOG4CXX_DEBUG(logger, "Configuring for " << vm["detector"].as<string>() << " detector");
        }

        if (no_client && vm.count("ctrl"))
        {
            LOG4CXX_DEBUG(logger, "Setting control endpoint to: " << vm["ctrl"].as<string>());
        }

        if (no_client && vm.count("output"))
        {
            LOG4CXX_DEBUG(logger, "Writing frames to file: " << vm["output"].as<string>());
        }

        if (no_client && vm.count("processes"))
        {
            LOG4CXX_DEBUG(logger, "Number of concurrent filewriter processes: " << vm["processes"].as<unsigned int>());
        }

        if (no_client && vm.count("rank"))
        {
            LOG4CXX_DEBUG(logger, "This process rank (index): " << vm["rank"].as<unsigned int>());
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

void configureDefaults(boost::shared_ptr<filewriter::FileWriterController> fwc, po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  cfg.set_param<string>("fr_setup/fr_shared_mem", vm["sharedbuf"].as<string>());
  cfg.set_param<string>("fr_setup/fr_ready_cnxn", vm["ready"].as<string>());
  cfg.set_param<string>("fr_setup/fr_release_cnxn", vm["release"].as<string>());
  cfg.set_param<string>("output", vm["output"].as<string>());
  
  fwc->configure(cfg, reply);
}

void configurePercival(boost::shared_ptr<filewriter::FileWriterController> fwc) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  cfg.set_param<string>("plugin/load/library", "./lib/libPercivalProcessPlugin.so");
  cfg.set_param<string>("plugin/load/index", "percival");
  cfg.set_param<string>("plugin/load/name", "PercivalProcessPlugin");
  cfg.set_param<string>("plugin/connect/index", "percival");
  cfg.set_param<string>("plugin/connect/connection", "frame_receiver");
  
  fwc->configure(cfg, reply);
}

void configureExcalibur(boost::shared_ptr<filewriter::FileWriterController> fwc) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  cfg.set_param<string>("plugin/load/library", "./lib/libExcaliburReorderPlugin.so");
  cfg.set_param<string>("plugin/load/index", "excalibur");
  cfg.set_param<string>("plugin/load/name", "ExcaliburReorderPlugin");
  cfg.set_param<string>("plugin/connect/index", "excalibur");
  cfg.set_param<string>("plugin/connect/connection", "frame_receiver");
  cfg.set_param<string>("excalibur/bitdepth", "12-bit");
  
  fwc->configure(cfg, reply);
}

void configureHDF5(boost::shared_ptr<filewriter::FileWriterController> fwc, string input) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  cfg.set_param<string>("plugin/load/library", "./lib/libHdf5Plugin.so");
  cfg.set_param<string>("plugin/load/index", "hdf");
  cfg.set_param<string>("plugin/load/name", "FileWriter");
  cfg.set_param<string>("plugin/connect/index", "hdf");
  cfg.set_param<string>("plugin/connect/connection", input);
  
  fwc->configure(cfg, reply);
}

void configurePercivalDataset(boost::shared_ptr<filewriter::FileWriterController> fwc, string name, bool master=false) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  rapidjson::Document jsonDoc;
  rapidjson::Value dims(rapidjson::kArrayType);
  rapidjson::Document::AllocatorType& dimAllocator = jsonDoc.GetAllocator();
  jsonDoc.SetObject();
  dims.PushBack(1484, dimAllocator);
  dims.PushBack(1408, dimAllocator);
  
  rapidjson::Document::AllocatorType& chunkAllocator = jsonDoc.GetAllocator();
  rapidjson::Value chunks(rapidjson::kArrayType);
  chunks.PushBack(1, chunkAllocator);
  chunks.PushBack(1484, chunkAllocator);
  chunks.PushBack(704, chunkAllocator);
  
  cfg.set_param<string>("hdf/dataset/cmd", "create");
  cfg.set_param<string>("hdf/dataset/name", name);
  cfg.set_param<int>("hdf/dataset/datatype", 1);
  cfg.set_param<rapidjson::Value>("hdf/dataset/dims", dims);
  cfg.set_param<rapidjson::Value>("hdf/dataset/chunks", chunks);
  if (master) {
    cfg.set_param<string>("hdf/master", name);
  }
  
  fwc->configure(cfg, reply);
}

void configureExcaliburDataset(boost::shared_ptr<filewriter::FileWriterController> fwc, string name) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  rapidjson::Document jsonDoc;
  rapidjson::Value dims(rapidjson::kArrayType);
  rapidjson::Document::AllocatorType& dimAllocator = jsonDoc.GetAllocator();
  jsonDoc.SetObject();
  dims.PushBack(256, dimAllocator);
  dims.PushBack(2048, dimAllocator);
  
  cfg.set_param<string>("hdf/dataset/cmd", "create");
  cfg.set_param<string>("hdf/dataset/name", name);
  cfg.set_param<int>("hdf/dataset/datatype", 1);
  cfg.set_param<rapidjson::Value>("hdf/dataset/dims", dims);
  
  fwc->configure(cfg, reply);
}

void configureEigerDataset(boost::shared_ptr<filewriter::FileWriterController> fwc, string name) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  rapidjson::Document jsonDoc;
  rapidjson::Value dims(rapidjson::kArrayType);
  rapidjson::Document::AllocatorType& dimAllocator = jsonDoc.GetAllocator();
  jsonDoc.SetObject();
  dims.PushBack(2167, dimAllocator);
  dims.PushBack(2070, dimAllocator);
  
  cfg.set_param<string>("hdf/dataset/cmd", "create");
  cfg.set_param<string>("hdf/dataset/name", name);
  cfg.set_param<int>("hdf/dataset/datatype", 1);
  cfg.set_param<rapidjson::Value>("hdf/dataset/dims", dims);
  cfg.set_param<string>("hdf/dataset/compression", "lz4");
  
  fwc->configure(cfg, reply);
}

void configureFileWriter(boost::shared_ptr<filewriter::FileWriterController> fwc, po::variables_map vm) {
  OdinData::IpcMessage cfg;
  OdinData::IpcMessage reply;
  
  cfg.set_param<string>("hdf/file/name", vm["output"].as<string>());
  cfg.set_param<string>("hdf/file/path", "/tmp/");
  cfg.set_param<unsigned int>("hdf/frames", vm["frames"].as<unsigned int>());
  cfg.set_param<bool>("hdf/write", true);
  
  fwc->configure(cfg, reply);
}

void configurePlugins(boost::shared_ptr<filewriter::FileWriterController> fwc, string detector) {
  if (detector == "excalibur") {
    configureExcalibur(fwc);
    configureHDF5(fwc, detector);
    configureExcaliburDataset(fwc, "data");
  }
  else if (detector == "percival") {
    configurePercival(fwc);
    configureHDF5(fwc, detector);
    configurePercivalDataset(fwc, "data", true);
    configurePercivalDataset(fwc, "reset");
  }
  else if (detector == "eiger") {
    configureHDF5(fwc, "frame_receiver");
    configureEigerDataset(fwc, "data");
  }
}

int main(int argc, char** argv)
{
  LoggerPtr logger(Logger::getLogger("FW.App"));
  
  try {

    // Create a default basic logger configuration, which can be overridden by command-line option later
    BasicConfigurator::configure();

    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    boost::shared_ptr<filewriter::FileWriterController> fwc;
    fwc = boost::shared_ptr<filewriter::FileWriterController>(new filewriter::FileWriterController());

    // Configure the control channel for the filewriter
    OdinData::IpcMessage cfg;
    OdinData::IpcMessage reply;
    cfg.set_param<std::string>("ctrl_endpoint", vm["ctrl"].as<string>());
    cfg.set_param<unsigned int>("frames", vm["frames"].as<unsigned int>());
    cfg.set_param<bool>("single-shot", vm["single-shot"].as<bool>());
    fwc->configure(cfg, reply);
    
    if (vm["no-client"].as<bool>()) {
      LOG4CXX_DEBUG(logger, "Adding configuration options to work without a client controller");
      configureDefaults(fwc, vm);
      configurePlugins(fwc, vm["detector"].as<string>());
      configureFileWriter(fwc, vm);
    }
    
    fwc->run();
    
    LOG4CXX_DEBUG(logger, "FileWriterController run finished. Stopping app.");

  } catch (const std::exception& e){
    LOG4CXX_ERROR(logger, e.what());
    // Nothing to do, terminate gracefully(?)
  }
  return 0;
}
