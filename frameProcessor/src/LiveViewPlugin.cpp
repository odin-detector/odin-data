/*
 * LiveImageViewPlugin.cpp
 *
 *  Created on: 6 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#include "LiveViewPlugin.h"
#include <boost/algorithm/string.hpp>

namespace FrameProcessor
{
/*Default Config*/
const int32_t     LiveViewPlugin::DEFAULT_FRAME_FREQ = 2;
const std::string LiveViewPlugin::DEFAULT_IMAGE_VIEW_SOCKET_ADDR = "tcp://*:1337";
const int32_t     LiveViewPlugin::DEFAULT_PER_SECOND = 2;
const std::string LiveViewPlugin::DEFAULT_DATASET_NAME = "";

/*Config Names*/
const std::string LiveViewPlugin::CONFIG_FRAME_FREQ =  "frame_frequency";
const std::string LiveViewPlugin::CONFIG_SOCKET_ADDR = "live_view_socket_addr";
const std::string LiveViewPlugin::CONFIG_PER_SECOND =  "per_second";
const std::string LiveViewPlugin::CONFIG_DATASET_NAME = "dataset_name";

/*Enum style arrays for the header*/
const std::string LiveViewPlugin::DATA_TYPES[] = {"uint8","uint16","uint32"};
const std::string LiveViewPlugin::COMPRESS_TYPES[] = {"none","LZ4","BSLZ4"};

/**
 * Constructor for this class
 */
LiveViewPlugin::LiveViewPlugin() :
    publish_socket(ZMQ_PUB)
{
  logger_ = Logger::getLogger("FW.LiveViewPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "LiveViewPlugin constructor.");

  setFrameFreqConfig(DEFAULT_FRAME_FREQ);
  setPerSecondConfig(DEFAULT_PER_SECOND);
  setSocketAddrConfig(DEFAULT_IMAGE_VIEW_SOCKET_ADDR);
  setDatasetNameConfig(DEFAULT_DATASET_NAME);
}

/**
 * Destructor.
 */
LiveViewPlugin::~LiveViewPlugin()
{
  LOG4CXX_TRACE(logger_, "LiveViewPlugin destructor.");
  publish_socket.unbind(image_view_socket_addr);
}

/**
 * Process recieved frame. For the live view plugin, this means taking a copy if its 1 in N frames,
 * passing on the frame, and then transmitting the frame to the live view socket, either as raw data
 * or as an image.
 */
void LiveViewPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  LOG4CXX_TRACE(logger_, "LiveViewPlugin Process Frame.");

  std::string frame_dataset = frame->get_dataset_name();
  //if datasets is empty, or contains the frame's dataset, then we can print it
  if(datasets.empty() || std::find(datasets.begin(), datasets.end(), frame_dataset) != datasets.end()){

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    int32_t frame_num = frame->get_frame_number();
    int32_t elapsed_time = (now - time_last_frame).total_milliseconds();

    if(per_second != 0 && elapsed_time > time_between_frames) //time between frames too large, showing frame no matter what the frame number is
    {
      LOG4CXX_TRACE(logger_, "Elapsed time " << elapsed_time << " > " << time_between_frames);
      PassLiveFrame(frame, frame_num);
    }
    else if(frame_freq != 0 && frame_num % frame_freq == 0)
    {
      LOG4CXX_TRACE(logger_, "LiveViewPlugin Frame " << frame_num << " to be displayed.");
      PassLiveFrame(frame, frame_num);
    }
  }
  else
  {
    LOG4CXX_TRACE(logger_,"Frame dataset :" << frame_dataset << " not desired");
  }

  LOG4CXX_TRACE(logger_, "Pushing Data Frame" );
  //push frame down the pipeline no matter if frame was passed to live viewer or not
  this->push(frame);
}

void LiveViewPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try{
    //check if we're setting the frequency of frames to show
    if(config.has_param(CONFIG_FRAME_FREQ))
    {
      setFrameFreqConfig(config.get_param<int32_t>(CONFIG_FRAME_FREQ));
    }
    if(config.has_param(CONFIG_PER_SECOND))
    {
      setPerSecondConfig(config.get_param<int32_t>(CONFIG_PER_SECOND));
    }
    //check if we're setting the address of the socket to send the live view frames to.
    if(config.has_param(CONFIG_SOCKET_ADDR))
    {
      setSocketAddrConfig(config.get_param<std::string>(CONFIG_SOCKET_ADDR));
    }

    if(config.has_param(CONFIG_DATASET_NAME))
    {
      setDatasetNameConfig(config.get_param<std::string>(CONFIG_DATASET_NAME));
    }

    if(per_second == 0 && frame_freq == 0)
    {
      LOG4CXX_WARN(logger_, "CURRENT LIVE VIEW CONFIGURATION RESULTS IN IT DOING NOTHING");
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
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_FRAME_FREQ, frame_freq);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_SOCKET_ADDR, image_view_socket_addr);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_PER_SECOND, per_second);
}

std::string LiveViewPlugin::getTypeFromEnum(int32_t type)
{
  if(type >= 0 && type < sizeof(DATA_TYPES)/sizeof(DATA_TYPES[0])){
    return DATA_TYPES[type];
  }else{
    return "unknown";
  }
}
/* -1: Unset, 0: None, 1: LZ4, 2: BSLZ4*/
std::string LiveViewPlugin::getCompressFromEnum(int32_t compress)
{
  if(compress >= 0 && compress < sizeof(COMPRESS_TYPES)/sizeof(COMPRESS_TYPES[0])){
    return COMPRESS_TYPES[compress];
  }else{
    return COMPRESS_TYPES[0];
  }
}

void LiveViewPlugin::PassLiveFrame(boost::shared_ptr<Frame> frame, int32_t frame_num)
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
  publish_socket.send(buffer.GetString(), ZMQ_SNDMORE);
  LOG4CXX_TRACE(logger_, "LiveViewPlugin Sending frame raw data");
  publish_socket.send(size, frame_data_copy, 0);

  time_last_frame = boost::posix_time::microsec_clock::local_time();
}

void LiveViewPlugin::setPerSecondConfig(int32_t value)
{
  per_second = value;

  if(per_second == 0)
  {
    LOG4CXX_INFO(logger_, "Disabling Frames Per Second Option");
  }
  else
  {
    LOG4CXX_INFO(logger_, "Displaying " << per_second << " frames per second");
    time_between_frames = 1000 / per_second;
  }
}

void LiveViewPlugin::setFrameFreqConfig(int32_t value)
{
  frame_freq = value;
  if(frame_freq == 0)
  {
    LOG4CXX_INFO(logger_, "Disabling Frame Frequency");
  }
  else
  {
    LOG4CXX_INFO(logger_, "Showing every " << frame_freq << " frame(s)");
  }
}

void LiveViewPlugin::setSocketAddrConfig(std::string value)
{
  //we dont want to unbind and rebind the same address or it causes errors, so we check first
  if(publish_socket.has_bound_endpoint(value))
  {
    LOG4CXX_WARN(logger_, "Socket already bound to " << value << ". Doing nothing");
    return;
  }
  publish_socket.unbind(image_view_socket_addr);

  image_view_socket_addr = value;
  LOG4CXX_INFO(logger_, "Setting Live View Socket Address to " << image_view_socket_addr);
  publish_socket.bind(image_view_socket_addr);
}

void LiveViewPlugin::setDatasetNameConfig(std::string value)
{
  std::string delim = ",";
  datasets.clear();
  if(!value.empty())
  {
    //delim value string by comma
    boost::split(datasets, value, boost::is_any_of(delim));
  }
  std::string dataset_string = "";
  for(int i = 0; i< datasets.size(); i++){
    boost::trim(datasets[i]);
    dataset_string += datasets[i] + ",";
  }
  LOG4CXX_INFO(logger_, "Setting the datasets allowed to: " << dataset_string);
}
}/*namespace FrameProcessor*/



