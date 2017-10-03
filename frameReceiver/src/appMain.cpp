/*!
 * appMain.cpp
 *
 *  Created on: Nov 5, 2014
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverApp.h"
#include "logging.h"
#include <signal.h>
#include <iostream>

// Interrupt signal handler
void intHandler (int sig)
{
  FrameReceiver::FrameReceiverApp::stop ();
}

// Main application entry point

int main (int argc, char** argv)
{
  int rc = 0;

  // Trap Ctrl-C and pass to TestThroughputMulti
  signal (SIGINT, intHandler);
  signal (SIGTERM, intHandler);

  // Set the application path and locale for logging
  setlocale(LC_CTYPE, "UTF-8");
  OdinData::app_path = argv[0];

  // Create a FrameReceiverApp instance
  FrameReceiver::FrameReceiverApp fr_instance;

  // Parse command line arguments and set up node configuration
  rc = fr_instance.parse_arguments (argc, argv);

  if (rc == 0)
  {
    // Run the instance
    fr_instance.run ();
  }

  return rc;

}
