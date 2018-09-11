/*
 * LiveImageViewPlugin.cpp
 *
 *  Created on: 6 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#include "LiveViewPlugin.h"

namespace FrameProcessor
{

/**
 * Constructor for this class
 */
LiveViewPlugin::LiveViewPlugin() :
    //show_raw_frame_(DEFAULT_SHOW_RAW_FRAME),
    frame_freq_(DEFAULT_FRAME_FREQ),
    image_view_socket_addr_(DEFAULT_IMAGE_VIEW_SOCKET_ADDR),
    publish_socket_(ZMQ_SUB)
{
  logger_ = Logger::getLogger("FW.LiveViewPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "LiveViewPlugin constructor.");

  publish_socket_.bind(image_view_socket_addr_);
}

/**
 * Destructor.
 */
LiveViewPlugin::~LiveViewPlugin()
{
  LOG4CXX_TRACE(logger_, "LiveViewPlugin destructor.");
  publish_socket_.unbind(image_view_socket_addr_);
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
  {
    void* frame_copy = (void*)frame->get_data();

    std::string aqqID = frame->get_acquisition_id();
    dimensions_t dim = frame->get_dimensions();
    int type = frame->get_data_type();
    size_t size = frame->get_data_size();

    rapidjson::Document document; //heaer info
    document.SetObject();

    //building image header
    rapidjson::Value keyFrame("Frame Number", document.GetAllocator());
    rapidjson::Value valueFrame(frame_num);
    document.AddMember(keyFrame, valueFrame, document.GetAllocator());

    rapidjson::Value keyAqq("Aqquisition ID", document.GetAllocator());
    rapidjson::Value valueAqq(aqqID.c_str(), document.GetAllocator());
    document.AddMember(keyAqq, valueAqq, document.GetAllocator());

    rapidjson::Value keyType("Data Type", document.GetAllocator());
    rapidjson::Value valueType(type);
    document.AddMember(keyType, valueType, document.GetAllocator());

    rapidjson::Value keySize("Data Size", document.GetAllocator());
    rapidjson::Value valueSize(size);
    document.AddMember(keySize, valueSize, document.GetAllocator());

    rapidjson::Value keyDims("Dimensions", document.GetAllocator());
    rapidjson::Value valueDims(dim);
    document.AddMember(keyDims, valueDims, document.GetAllocator());



    //convert to a json like string so that it can be passed along the socket as the image header
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);

    publish_socket_.send(buffer.GetString(), ZMQ_SNDMORE);
    publish_socket_.send(frame->get_data_size(), frame_copy, 0);

    this->push(frame); //finally, push frame once we are done with it (hopefully this plugin wont cause a backlog of frames to build up)

  }
  else
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
      LOG4CXX_INFO(logger_, "Setting Live View to show 1 in " << frame_freq_ << "Frames");
    }

    //check if we're setting the address of the socket to send the live view frames to.
    if(config.has_param(LV_LIVE_VIEW_SOCKET_ADDR_CONFIG))
    {
      //we need to unbind the socket before re-binding, otherwise it would potentially break
      publish_socket_.unbind(image_view_socket_addr_);
      image_view_socket_addr_ = config.get_param<std::string>(LV_LIVE_VIEW_SOCKET_ADDR_CONFIG);
      LOG4CXX_INFO(logger_, "Setting Live View Socket Address to " << image_view_socket_addr_);
      publish_socket_.bind(image_view_socket_addr_);
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
  reply.set_param(get_name() + "/" + LV_LIVE_VIEW_SOCKET_ADDR_CONFIG, image_view_socket_addr_);
}
}/*namespace FrameProcessor*/

