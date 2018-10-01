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
  void PassLiveFrame(boost::shared_ptr<Frame> frame);

  /* Default Config Values*/
  /** The default value for the Frame Frequency configuration*/
  static const int32_t     DEFAULT_FRAME_FREQ;
  /** The default value for the frames per second configuration*/
  static const int32_t     DEFAULT_PER_SECOND;
  /** The default value for the ZMQ socket address*/
  static const std::string DEFAULT_IMAGE_VIEW_SOCKET_ADDR;
  /** The default value for the dataset name filter*/
  static const std::string DEFAULT_DATASET_NAME;

  /* Config Names*/
  /** The name of the Frame Frequency config in the json file*/
  static const std::string CONFIG_FRAME_FREQ;
  /** The name of the Per Second config in the json file*/
  static const std::string CONFIG_PER_SECOND;
  /** The name of the Socket Address config in the json file*/
  static const std::string CONFIG_SOCKET_ADDR;
  /** The name of the Dataset Name config in the json file*/
  static const std::string CONFIG_DATASET_NAME;

private:

  /* Possible Data and Compression Types*/
  /** List of possible dtype strings*/
  static const std::string DATA_TYPES[];
  /** List of possible compression type strings*/
  static const std::string COMPRESS_TYPES[];

  void requestConfiguration(OdinData::IpcMessage& reply);
  std::string getTypeFromEnum(int32_t type);
  std::string getCompressFromEnum(int32_t compress);
  void setPerSecondConfig(int32_t value);
  void setFrameFreqConfig(int32_t value);
  void setSocketAddrConfig(std::string value);
  void setDatasetNameConfig(std::string value);


  /** Time between frames in milliseconds, calculated from the per_second config*/
  int32_t time_between_frames;
  /** Time the last frame was shown*/
  boost::posix_time::ptime time_last_frame;

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Every Nth Frame to display*/
  int32_t frame_freq;
  /** Address for ZMQ socket for transmitting images*/
  std::string image_view_socket_addr;
  /** Frames to show per second. Will override the frame_freq if elapsed time between frames gets too big*/
  int32_t per_second;
  /** The socket the live view image frames will be sent to*/
  OdinData::IpcChannel publish_socket;
  /** List of the dataset names to publish. If a frame comes in with a dataset name not on the list it will be ignored*/
  std::vector<std::string>datasets;

};

}/* Namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_*/
