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
#include "SharedMemoryParser.h"
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
                ("ready",       po::value<std::string>()->default_value("tcp://127.0.0.1:5001"),
                    "Ready ZMQ endpoint from frameReceiver")
                ("release",       po::value<std::string>()->default_value("tcp://127.0.0.1:5002"),
                        "Release frame ZMQ endpoint from frameReceiver")
                ("frames,f",     po::value<unsigned int>()->default_value(1),
                    "Set the number of frames to be notified about before terminating")
                ("sharedbuf",    po::value<std::string>()->default_value("FrameReceiverBuffer"),
                    "Set the name of the shared memory frame buffer")
                ("output,o",     po::value<std::string>(),
                    "Name of HDF5 file to write frames to (default: no file writing)")
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

        if (vm.count("ready"))
        {
            LOG4CXX_DEBUG(logger, "Setting frame ready notification ZMQ address to " << vm["ready"].as<string>());
        }

        if (vm.count("release"))
        {
            LOG4CXX_DEBUG(logger, "Setting frame release notification ZMQ address to " << vm["release"].as<string>());
        }

        if (vm.count("frames"))
        {
            LOG4CXX_DEBUG(logger, "Setting number of frames to receive to " << vm["frames"].as<unsigned int>());
        }

        if (vm.count("output"))
        {
            LOG4CXX_DEBUG(logger, "Writing frames to file: " << vm["output"].as<string>());
        }

        if (vm.count("processes"))
        {
            LOG4CXX_DEBUG(logger, "Number of concurrent filewriter processes: " << vm["processes"].as<unsigned int>());
        }

        if (vm.count("rank"))
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

int main(int argc, char** argv)
{
    // Create a default basic logger configuration, which can be overridden by command-line option later
    BasicConfigurator::configure();

    // Setup some rapid JSON objects that will be usefull for us
    rapidjson::Document jsonDoc;
    jsonDoc.SetObject();
    rapidjson::Document::AllocatorType& allocator = jsonDoc.GetAllocator();
    rapidjson::Value map;
    map.SetObject();
    rapidjson::Value val;

    LoggerPtr logger(Logger::getLogger("socketApp"));
    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    boost::shared_ptr<filewriter::FileWriterController> fwc;
    fwc = boost::shared_ptr<filewriter::FileWriterController>(new filewriter::FileWriterController());

    // Configure the control channel for the filewriter
    FrameReceiver::IpcMessage cfg;
    cfg.set_param<std::string>("ctrl_endpoint", "tcp://127.0.0.1:5004");
    fwc->configure(cfg);

    // Configure the shared memory interface for the filewriter
    val.SetObject();
    val.SetString(vm["release"].as<string>().c_str(), allocator);
    map.AddMember("fr_release_cnxn", val, allocator);
    val.SetObject();
    val.SetString(vm["ready"].as<string>().c_str(), allocator);
    map.AddMember("fr_ready_cnxn", val, allocator);
    val.SetObject();
    val.SetString(vm["sharedbuf"].as<string>().c_str(), allocator);
    map.AddMember("fr_shared_mem", val, allocator);
    rapidjson::Value fr_config;
    fr_config.SetObject();
    fr_config.AddMember("fr_setup", map, allocator);
    FrameReceiver::IpcMessage shm_cfg(fr_config);
    fwc->configure(shm_cfg);

    // Now load the percival plugin
    map.SetObject();
    map.AddMember("plugin_library", "./lib/libPercivalProcessPlugin.so", allocator);
    map.AddMember("plugin_index", "percival", allocator);
    map.AddMember("plugin_name", "PercivalProcessPlugin", allocator);
    rapidjson::Value plugin_config;
    plugin_config.SetObject();
    plugin_config.AddMember("load_plugin", map, allocator);
    {
      FrameReceiver::IpcMessage plugin_msg(plugin_config);
      fwc->configure(plugin_msg);
    }

    // Connect the Percival plugin to the shared memory controller
    map.SetObject();
    map.AddMember("plugin_index", "percival", allocator);
    map.AddMember("plugin_connect_to", "frame_receiver", allocator);
    plugin_config.SetObject();
    plugin_config.AddMember("connect_plugin", map, allocator);
    {
      FrameReceiver::IpcMessage plugin_msg(plugin_config);
      fwc->configure(plugin_msg);
    }

    // Now load the HDF5 plugin
    map.SetObject();
    map.AddMember("plugin_library", "./lib/libHdf5Plugin.so", allocator);
    map.AddMember("plugin_index", "hdf", allocator);
    map.AddMember("plugin_name", "FileWriter", allocator);
    plugin_config.SetObject();
    plugin_config.AddMember("load_plugin", map, allocator);
    {
      FrameReceiver::IpcMessage plugin_msg(plugin_config);
      fwc->configure(plugin_msg);
    }

    // Connect the HDF5 plugin to the Percival plugin
    map.SetObject();
    map.AddMember("plugin_index", "hdf", allocator);
    map.AddMember("plugin_connect_to", "percival", allocator);
    plugin_config.SetObject();
    plugin_config.AddMember("connect_plugin", map, allocator);
    {
      FrameReceiver::IpcMessage plugin_msg(plugin_config);
      fwc->configure(plugin_msg);
    }

    // Now wait for the shutdown
    fwc->waitForShutdown();

    return 0;
}
