/*
 * LiveImageViewPlugin.cpp
 *
 *  Created on: 6 Sept 2018
 *      Author: wbd45595
 */

#include "LiveViewPlugin.h"

namespace FrameProcessor
{

/**
 * Constructor for this class
 */
LiveViewPlugin::LiveViewPlugin() :
    show_raw_frame_(DEFAULT_SHOW_RAW_FRAME),
    frame_freq_(DEFAULT_FRAME_FREQ),
    image_view_socket_addr_(DEFAULT_IMAGE_VIEW_SOCKET_ADDR)
{
  logger_ = Logger::getLogger("FW.LiveViewPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "LiveViewPlugin constructor.");

  //TODO: init ZMQ socket to publish image
}

/**
 * Destructor.
 */
LiveViewPlugin::~LiveViewPlugin()
{
  LOG4CXX_TRACE(logger_, "LiveViewPlugin destructor.");
}

/**
 * Process recieved frame. For the live view plugin, this means taking a copy if its 1 in N frames,
 * passing on the frame, and then transmitting the frame to the live view socket, either as raw data
 * or as an image.
 */
void LiveViewPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  int frame_num = frame->get_frame_number();
  if ((frame_num % frame_freq_) == 0)
  { //get copy of frame if 1 in N
    Frame frame_copy = frame;
    this->push(frame);

    if (show_raw_frame_)
    {
      //TODO: publish frame on image socket
    } else
    {
      //TODO: convert to an image, then publish
    }
  } else
  {
    this->push(frame);
  }
}

void LiveViewPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try{
    //check if we're setting the frequency of frames to show
    if(config.has_param(LV_FRAME_FREQ_CONFIG))
    {
      frame_freq_ = config.get_param<int32_t>(LV_FRAME_FREQ_CONFIG);
      LOG4CXX_INFO(logger_, "Setting Live View to show every " << frame_freq_ << "th/nd/st Frame");
    }

    //check if we are setting the raw frame transmittion on/off
    if(config.has_param(LV_RAW_FRAME_CONFIG))
    {
      show_raw_frame_ = config.get_param<bool>(LV_RAW_FRAME_CONFIG);
      LOG4CXX_INFO(logger_, "Setting Raw Frame Mode to " << show_raw_frame_);
    }

    if(config.has_param(LV_LIVE_VIEW_SOCKET_ADDR_CONFIG))
    {
      image_view_socket_addr_ = config.get_param<std::string>(LV_LIVE_VIEW_SOCKET_ADDR_CONFIG);
      LOG4CXX_INFO(logger_, "Setting Live View Socket Address to " << image_view_socket_addr_);
      //TODO: socket address change, redo the socket setup maybe?
    }
  }
  catch (std::runtime_error& e)
  {
    std::stringstream ss;
    ss << "Bad ctrl msg: " << e.what();
    this->set_error(ss.str());
    throw;
  }
}

void LiveViewPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(get_name() + "/" + LV_FRAME_FREQ_CONFIG, frame_freq_);
  reply.set_param(get_name() + "/" + LV_RAW_FRAME_CONFIG, show_raw_frame_);
  reply.set_param(get_name() + "/" + LV_LIVE_VIEW_SOCKET_ADDR_CONFIG, image_view_socket_addr_);
}
}/*namespace FrameProcessor*/

