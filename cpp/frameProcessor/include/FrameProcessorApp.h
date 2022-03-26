/*
 * FrameProcessorApp.h
 *
 *  Created on: Dec 18. 2020
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef FRAMEPROCESSORAPP_H_
#define FRAMEPROCESSORAPP_H_

#include "DebugLevelLogger.h"
#include "FrameProcessorController.h"

namespace FrameProcessor
{
class FrameProcessorApp
{

public:

  FrameProcessorApp();
  ~FrameProcessorApp();

  int parse_arguments(int argc, char** argv);
  void configure_controller(OdinData::IpcMessage& config_msg);
  void run();

private:

  LoggerPtr logger_;                    //!< Log4CXX logger instance pointer
  static boost::shared_ptr<FrameProcessorController> controller_; //!< FrameProcessor controller object

  // Command line options
  unsigned int io_threads_;  //!< Number of IO threads for IPC channels
  std::string ctrl_channel_endpoint_;  //!< IPC channel endpoint for control communication with other processes
  std::string config_file_; //!< Full path to JSON configuration file

};

} // namespace FrameProcessor
#endif /* FRAMEPROCESSOR_APP_H */
