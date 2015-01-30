/*
 * FrameReceiver.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverApp.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <iterator>
using namespace std;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
namespace pt = boost::property_tree;

using namespace FrameReceiver;

FrameReceiverApp::FrameReceiverApp(void)
{

	// Retreive a logger instance
	mLogger = Logger::getLogger("FrameReceiver");
}


FrameReceiverApp::~FrameReceiverApp()
{

}

int FrameReceiverApp::parseArguments(int argc, char** argv)
{

	int rc = 0;
	try
	{
		// Create and parse options
		po::options_description prog_desc("Program options");
		prog_desc.add_options()
				("help",
					"Print this help message")
				("node,n",      po::value<unsigned int>()->default_value(1),
					"Set node ID")
				("config,c",    po::value<string>(),
					"Specify program configuration file")
				("logconfig,l", po::value<string>(),
					"Specify log4cxx configuration file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, prog_desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			cout << prog_desc << endl;
			exit(0);
		}
		if (vm.count("logconfig"))
		{
			PropertyConfigurator::configure(vm["logconfig"].as<string>());
			LOG4CXX_DEBUG(mLogger, "log4cxx config file is set to " << vm["logconfig"].as<string>());
		}
	}
	catch (Exception &e)
	{
		LOG4CXX_ERROR(mLogger, "Got Log4CXX exception: " << e.what());
		rc = 1;
	}
	catch (exception &e)
	{
		LOG4CXX_ERROR(mLogger, "Got exception:" << e.what());
		rc = 1;
	}
	catch (...)
	{
		LOG4CXX_ERROR(mLogger, "Exception of unknown type!");
		rc = 1;
	}

	return rc;

}


void FrameReceiverApp::run(void)
{

	LOG4CXX_INFO(mLogger,  "Running frame receiver");

}

void FrameReceiverApp::stop(void)
{

}


