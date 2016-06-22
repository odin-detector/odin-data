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

#include <JSONSubscriber.h>
#include "FileWriterController.h"
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

    LoggerPtr logger(Logger::getLogger("socketApp"));
    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    //DataBlockPool::allocate(200, 1024);

    boost::shared_ptr<filewriter::FileWriterController> fwc;
    fwc = boost::shared_ptr<filewriter::FileWriterController>(new filewriter::FileWriterController());
    //filewriter::JSONSubscriber sh("tcp://127.0.0.1:5003");
    //sh.registerCallback(fwc);
    //sh.subscribe();

    // Configure the control channel for the filewriter
    std::string cfgString = "{" + std::string("\"ctrl_endpoint\":\"tcp://127.0.0.1:5004\"") + "}";
    boost::shared_ptr<JSONMessage> cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Configure the shared memory setup of the file writer controller
    cfgString = "{" +
        std::string("\"fr_release_cnxn\":\"") + vm["release"].as<string>() + std::string("\",") +
        "\"fr_ready_cnxn\":\"" + vm["ready"].as<string>() + "\"," +
        "\"fr_shared_mem\":\"" + vm["sharedbuf"].as<string>() + "\"" +
        "}";
    cfgString = "{\"fr_setup\": " + cfgString + "}";
    cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Now load the percival plugin
    cfgString = "{" +
            std::string("\"plugin_library\":\"./lib/libPercivalProcessPlugin.so\",") +
            "\"plugin_index\":\"percival\"," +
            "\"plugin_name\":\"PercivalProcessPlugin\"" +
            "}";
    cfgString = "{\"load_plugin\": " + cfgString + "}";
    cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Connect the Percival plugin to the shared memory controller
    cfgString = "{" +
            std::string("\"plugin_index\":\"percival\",") +
            "\"plugin_connect_to\":\"frame_receiver\"" +
            "}";
    cfgString = "{\"connect_plugin\": " + cfgString + "}";
    cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Disconnect the HDF5 plugin from the shared memory controller
    cfgString = "{" +
            std::string("\"plugin_index\":\"hdf\",") +
            "\"plugin_disconnect_from\":\"frame_receiver\"" +
            "}";
    cfgString = "{\"disconnect_plugin\": " + cfgString + "}";
    cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Connect the HDF5 plugin to the Percival plugin
    cfgString = "{" +
            std::string("\"plugin_index\":\"hdf\",") +
            "\"plugin_connect_to\":\"percival\"" +
            "}";
    cfgString = "{\"connect_plugin\": " + cfgString + "}";
    cfg = boost::shared_ptr<JSONMessage>(new JSONMessage(cfgString));
    fwc->configure(cfg);

    // Now wait for the shutdown
    fwc->waitForShutdown();

    return 0;
}
