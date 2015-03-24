/*!
 * appMain.cpp
 *
 *  Created on: Nov 5, 2014
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverApp.h"
#include <signal.h>
#include <iostream>

// Interrupt signal handler
void intHandler(int sig)
{
	FrameReceiver::FrameReceiverApp::stop();
}

// Main application entry point

int main(int argc, char** argv)
{
	int rc = 0;

	// Trap Ctrl-C and pass to TestThroughputMulti
	signal(SIGINT, intHandler);
	signal(SIGTERM, intHandler);

	// Create a default basic logger configuration, which can be overridden by command-line option later
	BasicConfigurator::configure();

	// Create a FrameRecveiverApp instance
	FrameReceiver::FrameReceiverApp fr_instance;

	// Parse command line arguments and set up node configuration
	rc = fr_instance.parse_arguments(argc, argv);

	// Run the instance
	fr_instance.run();


	return rc;

}
