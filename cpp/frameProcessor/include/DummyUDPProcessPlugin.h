/*
 * DummyUDPProcessPlugin.h
 *
 *  Created on: 22 Jul 2019
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef FRAMEPROCESSOR_DUMMYUDPROCESSPLUGIN_H_
#define FRAMEPROCESSOR_DUMMYUDPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "DummyUDPDefinitions.h"
#include "ClassLoader.h"
#include "DataBlockFrame.h"

namespace FrameProcessor
{

/**
 * FrameProcessor plugin for dummy UDP data streams used in integration testing. This
 * plugin processes incoming frames into output 2D images, handling lost packets 
 * appropriately. The plugin can be configured to either copy the incoming frame into a new
 * output frame, or set metadata etc on the incoming frame and push that out.
 */
class DummyUDPProcessPlugin : public FrameProcessorPlugin
{
public:
  DummyUDPProcessPlugin();
  virtual ~DummyUDPProcessPlugin();
  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestConfiguration(OdinData::IpcMessage& reply);
  void execute(const std::string& command, OdinData::IpcMessage& reply);
  std::vector<std::string> requestCommands();
  void status(OdinData::IpcMessage& status);
  bool reset_statistics(void);

private:

  /** Configuration constant for image width **/
  static const std::string CONFIG_IMAGE_WIDTH;
  /** Configuration constant for image height **/
  static const std::string CONFIG_IMAGE_HEIGHT;
  /** Configuraiton constant for copy frame mode **/
  static const std::string CONFIG_COPY_FRAME;

  /** Command execution constant for print command **/
  static const std::string EXECUTE_PRINT;

  void process_frame(std::shared_ptr<Frame> frame);
  void process_lost_packets(std::shared_ptr<Frame>& frame);

  /** Pointer to logger */
  LoggerPtr logger_;

  /** Image width **/
  int image_width_;
  /** Image height **/
  int image_height_;
  /** Packet loss counter **/
  int packets_lost_;
  /** Copy frame mode flag **/
  bool copy_frame_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_DUMMYUDPROCESSPLUGIN_H_ */
