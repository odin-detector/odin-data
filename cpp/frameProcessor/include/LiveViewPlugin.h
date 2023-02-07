/*
 * LiveViewPlugin.h
 *
 *  Created on: 6 Sept 2018
 *      Author: Ashley Neaves
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
  void pass_live_frame(boost::shared_ptr<Frame> frame);
  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

  /*DEFAULT CONFIG VALUES*/
  /** The default value for the Frame Frequency configuration*/
  static const int32_t     DEFAULT_FRAME_FREQ;
  /** The default value for the frames per second configuration*/
  static const int32_t     DEFAULT_PER_SECOND;
  /** The default value for the ZMQ socket address*/
  static const std::string DEFAULT_IMAGE_VIEW_SOCKET_ADDR;
  /** The default value for the dataset name filter*/
  static const std::string DEFAULT_DATASET_NAME;
  /** The default value for the Tagged Filter*/
  static const std::string DEFAULT_TAGGED_FILTER;

  /*Config Names*/
  /** The name of the Frame Frequency config in the json file*/
  static const std::string CONFIG_FRAME_FREQ;
  /** The name of the Per Second config in the json file*/
  static const std::string CONFIG_PER_SECOND;
  /** The name of the Socket Address config in the json file*/
  static const std::string CONFIG_SOCKET_ADDR;
  /** The name of the Dataset Name config in the json file*/
  static const std::string CONFIG_DATASET_NAME;
  /** The name of the Tagged Filter config in the json file*/
  static const std::string CONFIG_TAGGED_FILTER_NAME;

private:

  /*Possible Data and Compression Types*/
  /** List of possible dtype strings*/
  static const std::string DATA_TYPES[];
  /** List of possible compression type strings*/
  static const std::string COMPRESS_TYPES[];

  void add_json_member(rapidjson::Document *document, std::string key, std::string value);
  void add_json_member(rapidjson::Document *document, std::string key, uint32_t value);
  void requestConfiguration(OdinData::IpcMessage& reply);
  void set_per_second_config(int32_t value);
  void set_frame_freq_config(int32_t value);
  void set_socket_addr_config(std::string value);
  void set_dataset_name_config(std::string value);
  void set_tagged_filter_config(std::string value);


  /**time between frames in milliseconds, calculated from the per_second config*/
  //int32_t time_between_frames_;
  boost::posix_time::time_duration time_between_frames_;

  /**time the last frame was shown*/
  boost::posix_time::ptime time_last_frame_;

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
  /**List of the dataset names to publish. If a frame comes in with a dataset name not on the list it will be ignored*/
  std::vector<std::string> datasets_;
  /**List of Parameter names to look for. If a frame comes in without one of these tags it will be ignored*/
  std::vector<std::string> tags_;

  /**Boolean that shows if the plugin has a successfully bound ZMQ endpoint*/
  bool is_bound_;
};

}/* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_LIVEVIEWPLUGIN_H_*/
