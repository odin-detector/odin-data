/*
 * LiveViewPlugin.h
 *
 *  Created on: 6 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#ifndef FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_
#define FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;


#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

const int32_t DEFAULT_FRAME_FREQ = 2; //every 2nd frame will be displayed
//const bool DEFAULT_SHOW_RAW_FRAME = true; //by default, transmit the raw frame rather than converting to image TODO: bool or enum for different
const std::string DEFAULT_IMAGE_VIEW_SOCKET_ADDR = "tcp://*:1337";
static const std::string LV_FRAME_FREQ_CONFIG = "frame_frequency";
static const std::string LV_LIVE_VIEW_SOCKET_ADDR_CONFIG = "live_view_socket_addr";

const std::string DATA_TYPES[] = {"uint8","uint16","uint32"};
const std::string COMPRESS_TYPES[] = {"none","LZ4","BSLZ4"};

class LiveViewPlugin : public FrameProcessorPlugin{

public:
  LiveViewPlugin();
  virtual ~LiveViewPlugin();
  void process_frame(boost::shared_ptr<Frame> frame);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);

private:
  void requestConfiguration(OdinData::IpcMessage& reply);
  std::string getTypeFromEnum(int type);
  std::string getCompressFromEnum(int compress);


  /** Pointer to logger */
  LoggerPtr logger_;
  /** Every Nth Frame to display*/
  int frame_freq_;
  /** transmit a raw frame or compile to an image first */
  //bool show_raw_frame_;
  /** address for ZMQ socket for transmitting images*/
  std::string image_view_socket_addr_;

  OdinData::IpcChannel publish_socket_;

};

}/* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_*/
