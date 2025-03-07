/*
 * FrameReceiver.h
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERAPP_H_
#define FRAMERECEIVERAPP_H_

#include "DebugLevelLogger.h"
#include "FrameReceiverController.h"

using namespace OdinData;

namespace FrameReceiver
{

//! Frame receiver application class
//!
//! This class implements the main functionality of the FrameReceiver application, parsing command line
//! and configuraiton file options before creating, configuring and running the controller.

class FrameReceiverApp
{

public:

  FrameReceiverApp();
  ~FrameReceiverApp();

  int parse_arguments(int argc, char** argv);

  int run(void);
  static void stop(void);

private:

  LoggerPtr logger_;  //!< Log4CXX logger instance pointer
  static std::shared_ptr<FrameReceiverController> controller_;  //!< FrameReceiver controller object

  // Command line options
  unsigned int io_threads_;  //!< Number of IO threads for IPC channels
  std::string ctrl_channel_endpoint_;  //!< IPC channel endpoint for control communication with other processes
  std::string config_file_;  //!< Full path to JSON configuration file

};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERAPP_H_ */
