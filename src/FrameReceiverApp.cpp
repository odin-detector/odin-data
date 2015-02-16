/*!
 * FrameReceiver.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverApp.h"
#include "FrameReceiverConfig.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
using namespace std;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

using namespace FrameReceiver;

bool FrameReceiverApp::run_frame_receiver_ = true;

//! Constructor for FrameReceiverApp class.
//!
//! This constructor initialises the FrameRecevierApp instance

FrameReceiverApp::FrameReceiverApp(void) :
    rx_channel_(ZMQ_PAIR),
    ctrl_channel_(ZMQ_REP)
{

	// Retrieve a logger instance
	logger_ = Logger::getLogger("FrameReceiver");

}

//! Destructor for FrameRecveiverApp class
FrameReceiverApp::~FrameReceiverApp()
{

    // Delete the RX thread object by resetting the scoped pointer, allowing the IPC channel
    // to be closed cleanly
    rx_thread_.reset();

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
//! \return return code, 0 if OK, 1 if option parsing failed

int FrameReceiverApp::parseArguments(int argc, char** argv)
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
				("node,n",       po::value<unsigned int>()->default_value(FrameReceiver::Defaults::default_node),
					"Set the frame receiver node ID")
				("logconfig,l",  po::value<string>(),
					"Set the log4cxx logging configuration file")
				("maxmem,m",     po::value<std::size_t>()->default_value(FrameReceiver::Defaults::default_max_buffer_mem),
					"Set the maximum amount of shared memory to allocate for frame buffers")
				("sensortype,s", po::value<std::string>()->default_value("unknown"),
					"Set the sensor type to receive frame data from")
				("port,p",       po::value<uint16_t>()->default_value(FrameReceiver::Defaults::default_rx_port),
                    "Set the port to receive frame data on")
                ("ipaddress,i",  po::value<std::string>()->default_value(FrameReceiver::Defaults::default_rx_address),
                    "Set the IP address of the interface to receive frame data on")
				;

		// Group the variables for parsing at the command line and/or from the configuration file
		po::options_description cmdline_options;
		cmdline_options.add(generic).add(config);
		po::options_description config_file_options;
		config_file_options.add(config);

		// Parse the command line options
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
		po::notify(vm);

		// If the command-line help option was given, print help and exit
		if (vm.count("help"))
		{
			std::cout << cmdline_options << std::endl;
			exit(0);
		}

		// If the command line version option was given, print version and exit
		if (vm.count("version"))
		{
			std::cout << "Will print version here" << std::endl;
			exit(0);
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
				LOG4CXX_DEBUG(logger_, "Parsing configuration file " << config_file);
				po::store(po::parse_config_file(config_ifs, config_file_options, true), vm);
				po::notify(vm);
			}
			else
			{
				LOG4CXX_ERROR(logger_, "Unable to open configuration file " << config_file << " for parsing");
				exit(1);
			}
		}

		if (vm.count("logconfig"))
		{
			PropertyConfigurator::configure(vm["logconfig"].as<string>());
			LOG4CXX_DEBUG(logger_, "log4cxx config file is set to " << vm["logconfig"].as<string>());
		}

		if (vm.count("maxmem"))
		{
		    config_.max_buffer_mem_ = vm["maxmem"].as<std::size_t>();
            LOG4CXX_DEBUG(logger_, "Setting maxmem to " << config_.max_buffer_mem_);
		}

		if (vm.count("sensortype"))
		{
		    std::string sensor_name = vm["sensortype"].as<std::string>();
		    config_.sensor_type_ = config_.map_sensor_name_to_type(sensor_name);
		    LOG4CXX_DEBUG(logger_, "Setting sensor type to " << sensor_name << " (" << config_.sensor_type_ << ")");
		}

		if (vm.count("port"))
		{
		    config_.rx_port_ = vm["port"].as<uint16_t>();
		    LOG4CXX_DEBUG(logger_, "Setting RX port to " << config_.rx_port_);
		}

		if (vm.count("ipaddress"))
		{
		    config_.rx_address_ = vm["ipaddress"].as<std::string>();
		    LOG4CXX_DEBUG(logger_, "Setting RX interface address to " << config_.rx_address_);
		}
	}
	catch (Exception &e)
	{
		LOG4CXX_ERROR(logger_, "Got Log4CXX exception: " << e.what());
		rc = 1;
	}
	catch (exception &e)
	{
		LOG4CXX_ERROR(logger_, "Got exception:" << e.what());
		rc = 1;
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger_, "Exception of unknown type!");
		rc = 1;
	}

	return rc;

}


void FrameReceiverApp::run(void)
{

    run_frame_receiver_ = true;
    LOG4CXX_INFO(logger_,  "Running frame receiver");

    // Bind the control channel
    ctrl_channel_.bind(config_.ctrl_channel_endpoint_);

    // Bind the RX thread channel
    rx_channel_.bind(config_.rx_channel_endpoint_);

    // Create the RX thread object
    rx_thread_.reset(new FrameReceiverRxThread( config_, logger_));


    LOG4CXX_DEBUG(logger_, "Main thread entering event loop");

	while (run_frame_receiver_)
	{
#ifdef TEMP_TEST_RX_THREAD
	    int loopCount = 0;
	    const int maxCount = 5;
	    while (++loopCount <= maxCount)
	    {
            std::stringstream message_ss;
            message_ss << "Hello " << loopCount;
            std::string message = message_ss.str();
            rx_channel_.send(message);
	    }

	    loopCount = 0;
	    while (++loopCount <= maxCount)
	    {
	        if (rx_channel_.poll(-1))
	        {
	            std::string reply = rx_channel_.recv();
	            LOG4CXX_DEBUG(logger_, "Main Thread got reply : " << reply);
	        }
	    }
        run_frame_receiver_ = false;
#endif

        if (ctrl_channel_.poll(1000))
        {
            std::string ctrl_req = ctrl_channel_.recv();
            LOG4CXX_DEBUG(logger_, "Got control channel request: " << ctrl_req);

            ctrl_channel_.send(ctrl_req);
        }
	}

	// Destroy the RX thread
	rx_thread_.reset();

}

void FrameReceiverApp::stop(void)
{
    run_frame_receiver_ = false;
}


