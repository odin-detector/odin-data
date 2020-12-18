/*
 * FrameReceiverApp.h
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

  bool isBloscRequired();

  void configureController();
  void configureDetector();
  void configureDetectorDecoder();
  void configureBlosc();
  void configureHDF5();
  void configureDataset(string name, bool master=false);
  void configurePlugins();
  void configureFileWriter();

  void checkNoClientArgs();

  void run();

private:

  LoggerPtr logger_;                    //!< Log4CXX logger instance pointer
  std::string json_config_file_;        //!< Full path to JSON configuration file
  static boost::shared_ptr<FrameProcessorController> controller_; //!< FrameProcessor controller object
  boost::program_options::variables_map vm_; //!< Boost program options variable map
};

} // namespace FrameProcessor
#endif /* FRAMEPROCESSOR_APP_H */