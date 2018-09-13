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
    publish_socket_(ZMQ_PUB)
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
  LOG4CXX_TRACE(logger_, "LiveViewPlugin Process Frame.");
  int frame_num = frame->get_frame_number();
  LOG4CXX_TRACE(logger_, "LiveViewPlugin Frame Number " << frame_num);
  if ((frame_num % frame_freq_) == 0)
  {
    LOG4CXX_TRACE(logger_, "LiveViewPlugin Frame " << frame_num << " to be displayed.");
    void* frame_data_copy = (void*)frame->get_data();

    std::string aqqID = frame->get_acquisition_id();
    dimensions_t dim = frame->get_dimensions();
    std::string type = getTypeFromEnum(frame->get_data_type());
    size_t size = frame->get_data_size();
    std::string compress = getCompressFromEnum(frame->get_compression());

    rapidjson::Document document; //header info
    document.SetObject();
    LOG4CXX_TRACE(logger_, "LiveViewPlugin Building Frame Header");
    //building image header

    //getting frame num
    rapidjson::Value keyFrame("Frame Number", document.GetAllocator());
    rapidjson::Value valueFrame(frame_num);
    document.AddMember(keyFrame, valueFrame, document.GetAllocator());

    //getting Aqquisition ID
    rapidjson::Value keyAqq("Acquisition ID", document.GetAllocator());
    rapidjson::Value valueAqq(aqqID.c_str(), document.GetAllocator());
    document.AddMember(keyAqq, valueAqq, document.GetAllocator());

    //getting data type
    rapidjson::Value keyType("Data Type", document.GetAllocator());
    rapidjson::Value valueType(type.c_str(), document.GetAllocator());
    document.AddMember(keyType, valueType, document.GetAllocator());

    //getting Data Size
    rapidjson::Value keySize("Data Size", document.GetAllocator());
    rapidjson::Value valueSize(size);
    document.AddMember(keySize, valueSize, document.GetAllocator());

    //getting compression
    rapidjson::Value keyCompress("Compression", document.GetAllocator());
    rapidjson::Value valueCompress(compress.c_str(), document.GetAllocator());
    document.AddMember(keyCompress, valueCompress, document.GetAllocator());

    //getting dimensions
    rapidjson::Value keyDims("Dimensions", document.GetAllocator());
    rapidjson::Value valueDims(rapidjson::kArrayType);

    size_t dim_size = dim.size();
    for(size_t i=0; i < dim_size; i++){
      std::string dimString = boost::to_string(dim[i]);
      rapidjson::Value dimStringVal(dimString.c_str(), document.GetAllocator());
      valueDims.PushBack(dimStringVal, document.GetAllocator());

    }

    document.AddMember(keyDims, valueDims, document.GetAllocator());



    //convert to a json like string so that it can be passed along the socket as the image header
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<> > writer(buffer);

    document.Accept(writer);

    LOG4CXX_TRACE(logger_, "LiveViewPlugin Header Built, sending down socket.");
    publish_socket_.send(buffer.GetString(), ZMQ_SNDMORE);
    LOG4CXX_TRACE(logger_, "LiveViewPlugin Sending frame raw data");
    publish_socket_.send(size, frame_data_copy, 0);

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

std::string LiveViewPlugin::getTypeFromEnum(int type)
{
  if(type >= 0 && type < sizeof(DATA_TYPES)/sizeof(DATA_TYPES[0])){
    return DATA_TYPES[type];
  }else{
    return "unset";
  }
}
/* -1: Unset, 0: None, 1: LZ4, 2: BSLZ4*/
std::string LiveViewPlugin::getCompressFromEnum(int compress)
{
  if(compress >= 0 && compress < sizeof(COMPRESS_TYPES)/sizeof(COMPRESS_TYPES[0])){
    return COMPRESS_TYPES[compress];
  }else{
    return "unset";
  }
}
}/*namespace FrameProcessor*/



