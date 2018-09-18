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

class LiveViewPlugin : public FrameProcessorPlugin{

public:
  LiveViewPlugin();
  virtual ~LiveViewPlugin();
  void process_frame(boost::shared_ptr<Frame> frame);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void PassLiveFrame(boost::shared_ptr<Frame> frame, int32_t frame_num);

private:


  /*DEFAULT CONFIG VALUES*/
  static const int32_t     DEFAULT_FRAME_FREQ;// = 2; //every 2nd frame will be displayed
  static const std::string DEFAULT_IMAGE_VIEW_SOCKET_ADDR;// = "tcp://*:1337";
  static const int32_t     DEFAULT_PER_SECOND;



  /*Config Names*/
  static const std::string CONFIG_FRAME_FREQ;// = "frame_frequency";
  static const std::string CONFIG_SOCKET_ADDR;// = "live_view_socket_addr";
  static const std::string CONFIG_PER_SECOND;

  /*Possible Data and Compression Types*/
  static const std::string DATA_TYPES[];
  static const std::string COMPRESS_TYPES[];

  void requestConfiguration(OdinData::IpcMessage& reply);
  std::string getTypeFromEnum(int32_t type);
  std::string getCompressFromEnum(int32_t compress);
  void setPerSecondConfig(int32_t value);
  void setFrameFreqConfig(int32_t value);
  void setSocketAddrConfig(std::string value);


  /**time between frames in milliseconds*/
  int32_t time_between_frames;
  /**time the last frame was shown*/
  boost::posix_time::ptime time_last_frame;

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Every Nth Frame to display*/
  int32_t frame_freq_;
  /** address for ZMQ socket for transmitting images*/
  std::string image_view_socket_addr_;
  /**Frames to show per second. Will override the frame_freq if elapsed time between frames gets too big*/
  int32_t per_second_;
  /**The socket the live view image frames will be sent to*/
  OdinData::IpcChannel publish_socket_;

};

}/* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_*/
