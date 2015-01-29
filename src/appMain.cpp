/*
 * appMain.cpp
 *
 *  Created on: Nov 5, 2014
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

//#include "TestThroughputMulti.h"
#include <signal.h>
#include <iostream>
#include <zmq.h>
#include "IpcMessage.h"

// Interrupt signal handler
//void intHandler(int sig)
//{
//	TestThroughputMulti::stop();
//}

// Main application entry point

int main(int argc, char** argv)
{
	int rc = 0;

	// Trap Ctrl-C and pass to TestThroughputMulti
	//	signal(SIGINT, intHandler);

	// Create a default basic logger configuration, which can be overridden by command-line option later
	//BasicConfigurator::configure();

	// Create a TestThrougputMulti instance
	//TestThroughputMulti ttm;

	// Parse command line arguments and set up node configuration
	//rc = ttm.parseArguments(argc, argv);

	// Run the instance
	//ttm.run();


	return rc;

}
