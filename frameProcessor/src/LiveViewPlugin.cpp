/*
 * LiveImageViewPlugin.cpp
 *
 *  Created on: 6 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#include "LiveViewPlugin.h"

namespace FrameProcessor
{
/*Default Config*/
const int32_t     LiveViewPlugin::DEFAULT_FRAME_FREQ = 2;
const std::string LiveViewPlugin::DEFAULT_IMAGE_VIEW_SOCKET_ADDR = "tcp://*:1337";
const bool        LiveViewPlugin::DEFAULT_PER_SECOND = true;

/*Config Names*/
const std::string LiveViewPlugin::CONFIG_FRAME_FREQ =  "frame_frequency";
const std::string LiveViewPlugin::CONFIG_SOCKET_ADDR = "live_view_socket_addr";
const std::string LiveViewPlugin::CONFIG_PER_SECOND =  "per_second";

/*Enum style arrays for the header*/
const std::string LiveViewPlugin::DATA_TYPES[] = {"uint8","uint16","uint32"};
const std::string LiveViewPlugin::COMPRESS_TYPES[] = {"none","LZ4","BSLZ4"};

/**
 * Constructor for this class
 */
LiveViewPlugin::LiveViewPlugin() :
    //show_raw_frame_(DEFAULT_SHOW_RAW_FRAME),
    frame_freq_(DEFAULT_FRAME_FREQ),
    image_view_socket_addr_(DEFAULT_IMAGE_VIEW_SOCKET_ADDR),
    publish_socket_(ZMQ_PUB),
    is_per_second_(DEFAULT_PER_SECOND)
{
  logger_ = Logger::getLogger("FW.LiveViewPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "LiveViewPlugin constructor.");
  if(is_per_second_ && frame_freq_ != 0)
  {
    time_between_frames = 1000 / frame_freq_;
    time_last_frame = boost::posix_time::microsec_clock::local_time();

  }

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
  if (frame_num % frame_freq_ == 0 && !is_per_second_)
  {
    LOG4CXX_TRACE(logger_, "LiveViewPlugin Frame " << frame_num << " to be displayed.");
    PassLiveFrame(frame, frame_num);

  }
  else if(is_per_second_)
  {
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    LOG4CXX_DEBUG(logger_, "time since last frame was shown: " << (now - time_last_frame).total_milliseconds());
    if((now - time_last_frame).total_milliseconds() > time_between_frames)
    {
      PassLiveFrame(frame, frame_num);
    }
  }

  //push frame down the pipeline no matter if frame was passed to live viewer or not
  this->push(frame);
}

void LiveViewPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try{
    //check if we're setting the frequency of frames to show
    if(config.has_param(CONFIG_FRAME_FREQ))
    {
      int freq = config.get_param<int32_t>(CONFIG_FRAME_FREQ);
      if(freq != 0)
      {
        frame_freq_ = config.get_param<int32_t>(CONFIG_FRAME_FREQ);
        LOG4CXX_INFO(logger_, "Setting Frame Frequency to " << frame_freq_);
      }else{
        frame_freq_ = DEFAULT_FRAME_FREQ;
        LOG4CXX_WARN(logger_, "Cannot set frame frequency to 0! Setting to default value of " << DEFAULT_FRAME_FREQ);
      }
    }

    //check if we're setting the address of the socket to send the live view frames to.
    if(config.has_param(CONFIG_SOCKET_ADDR))
    {
      //we need to unbind the socket before re-binding, otherwise it would potentially break
      publish_socket_.unbind(image_view_socket_addr_);
      image_view_socket_addr_ = config.get_param<std::string>(CONFIG_SOCKET_ADDR);
      LOG4CXX_INFO(logger_, "Setting Live View Socket Address to " << image_view_socket_addr_);
      publish_socket_.bind(image_view_socket_addr_);
    }

    if(config.has_param(CONFIG_PER_SECOND))
    {
      is_per_second_ = config.get_param<bool>(CONFIG_PER_SECOND);
      LOG4CXX_INFO(logger_, "LIVE VIEW DISPLAYING X FRAMES PER SECOND: " << is_per_second_);
      if(is_per_second_ && frame_freq_ != 0)
      {
        time_between_frames = 1 / frame_freq_;
      }
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
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_FRAME_FREQ, frame_freq_);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_SOCKET_ADDR, image_view_socket_addr_);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_PER_SECOND, is_per_second_);
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

void LiveViewPlugin::PassLiveFrame(boost::shared_ptr<Frame> frame, int frame_num)
{
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
  rapidjson::Value keyFrame("frame_num", document.GetAllocator());
  rapidjson::Value valueFrame(frame_num);
  document.AddMember(keyFrame, valueFrame, document.GetAllocator());

  //getting Acquisition ID
  rapidjson::Value keyAqq("acquisition_id", document.GetAllocator());
  rapidjson::Value valueAqq(aqqID.c_str(), document.GetAllocator());
  document.AddMember(keyAqq, valueAqq, document.GetAllocator());

  //getting data type
  rapidjson::Value keyType("dtype", document.GetAllocator());
  rapidjson::Value valueType(type.c_str(), document.GetAllocator());
  document.AddMember(keyType, valueType, document.GetAllocator());

  //getting Data Size
  rapidjson::Value keySize("dsize", document.GetAllocator());
  rapidjson::Value valueSize(size);
  document.AddMember(keySize, valueSize, document.GetAllocator());

  //getting compression
  rapidjson::Value keyCompress("compression", document.GetAllocator());
  rapidjson::Value valueCompress(compress.c_str(), document.GetAllocator());
  document.AddMember(keyCompress, valueCompress, document.GetAllocator());

  //getting dimensions
  rapidjson::Value keyDims("shape", document.GetAllocator());
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
}

}/*namespace FrameProcessor*/



