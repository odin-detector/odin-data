/*
 * LiveImageViewPlugin.cpp
 *
 *  Created on: 6 Sept 2018
 *      Author: Adam Neaves - wbd45595
 */

#include "LiveViewPlugin.h"
#include "version.h"
#include <boost/algorithm/string.hpp>

namespace FrameProcessor
{
/* Default Config*/
const int32_t     LiveViewPlugin::DEFAULT_FRAME_FREQ = 1;
const int32_t     LiveViewPlugin::DEFAULT_PER_SECOND = 0;
const std::string LiveViewPlugin::DEFAULT_IMAGE_VIEW_SOCKET_ADDR = "tcp://127.0.0.1:5020";
const std::string LiveViewPlugin::DEFAULT_DATASET_NAME = "";

/* Config Names*/
const std::string LiveViewPlugin::CONFIG_FRAME_FREQ =  "frame_frequency";
const std::string LiveViewPlugin::CONFIG_PER_SECOND =  "per_second";
const std::string LiveViewPlugin::CONFIG_SOCKET_ADDR = "live_view_socket_addr";
const std::string LiveViewPlugin::CONFIG_DATASET_NAME = "dataset_name";

/**
 * Constructor for this class. Sets up ZMQ pub socket and other default values for the config
 */
LiveViewPlugin::LiveViewPlugin() :
    publish_socket_(ZMQ_PUB)
{
  logger_ = Logger::getLogger("FW.LiveViewPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_INFO(logger_, "LiveViewPlugin version " << this->get_version_long() << " loaded");


  set_frame_freq_config(DEFAULT_FRAME_FREQ);
  set_per_second_config(DEFAULT_PER_SECOND);
  set_socket_addr_config(DEFAULT_IMAGE_VIEW_SOCKET_ADDR);
  set_dataset_name_config(DEFAULT_DATASET_NAME);
}

/**
 * Class Destructor. Closes the Publish socket
 */
LiveViewPlugin::~LiveViewPlugin()
{
  LOG4CXX_TRACE(logger_, "LiveViewPlugin destructor.");
  publish_socket_.close();
}

/**
 * Process recieved frame. For the live view plugin, this means checking if certain conditions are true (time elapsed, frame number, dataset name)
 * and then, if the conditions mean sending the frame, creating a json header and copying the data to send to the publisher socket.
 * 
 * \param[in] frame - pointer to a frame object.
 */
void LiveViewPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  /** Static Frame Count will increment each time this method is called, basically as a count of how many frames have been processed by the plugin*/
  static uint32_t frame_count_;
  LOG4CXX_TRACE(logger_, "LiveViewPlugin Process Frame.");

  std::string frame_dataset = frame->get_dataset_name();
  /* If datasets is empty, or contains the frame's dataset, then we can potentially send it*/
  if (datasets_.empty() || std::find(datasets_.begin(), datasets_.end(), frame_dataset) != datasets_.end())
  {

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    /*int32_t frame_num = frame->get_frame_number();*/
    int32_t elapsed_time = (now - time_last_frame_).total_milliseconds();

    if (per_second_ != 0 && elapsed_time > time_between_frames_) //time between frames too large, showing frame no matter what the frame number is
    {
      LOG4CXX_TRACE(logger_, "Elapsed time " << elapsed_time << " > " << time_between_frames_);
      pass_live_frame(frame);
    }
    else if (frame_freq_ != 0 && frame_count_ % frame_freq_ == 0)
    {
      LOG4CXX_TRACE(logger_, "LiveViewPlugin Frame " << frame->get_frame_number() << " to be displayed.");
      pass_live_frame(frame);
    }
  }
  else
  {
    LOG4CXX_TRACE(logger_,"Frame dataset: " << frame_dataset << " not desired");
  }

  LOG4CXX_TRACE(logger_, "Pushing Data Frame" );
  //push frame down the pipeline no matter if frame was passed to live viewer or not
  frame_count_ ++;
  this->push(frame);
}


/**
 * Set configuration options for this Plugin.
 *
 * This sets up the Live View Plugin according to the configuration IpcMessage
 * objects that are received.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void LiveViewPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  try{
    /* Check if we're setting the frequency of frames to show*/
    if(config.has_param(CONFIG_FRAME_FREQ))
    {
      set_frame_freq_config(config.get_param<int32_t>(CONFIG_FRAME_FREQ));
    }
    /* Check if we're setting the per_second config*/
    if(config.has_param(CONFIG_PER_SECOND))
    {
      set_per_second_config(config.get_param<int32_t>(CONFIG_PER_SECOND));
    }
    /* Check if we are setting the dataset name filter*/
    if(config.has_param(CONFIG_DATASET_NAME))
    {
      set_dataset_name_config(config.get_param<std::string>(CONFIG_DATASET_NAME));
    }

    /* Display warning if configuration sets the plugin to do nothing*/
    if(per_second_ == 0 && frame_freq_ == 0)
    {
      LOG4CXX_WARN(logger_, "CURRENT LIVE VIEW CONFIGURATION RESULTS IN IT DOING NOTHING");
    }
    /* Check if we're setting the address of the socket to send the live view frames to.*/
    if(config.has_param(CONFIG_SOCKET_ADDR))
    {
      set_socket_addr_config(config.get_param<std::string>(CONFIG_SOCKET_ADDR));
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

/**
 * Get the configuration values for this Plugin.
 *
 * \param[out] reply - Response IpcMessage.
 */
void LiveViewPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_FRAME_FREQ, frame_freq_);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_SOCKET_ADDR, image_view_socket_addr_);
  reply.set_param(get_name() + "/" + LiveViewPlugin::CONFIG_PER_SECOND, per_second_);
}

/**
 * Constructs a header with information about the data frame, then sends that header and the data
 * to the ZMQ socket interface to be consumed by an external live viewer.
 * The Header contains the following:
 * - int32_t Frame number
 * - string   Acquisition ID
 * - string   Data Type
 * - size_t   Data Size
 * - string   compression type
 * - size_t[] dimensions
 * \param[in] frame - pointer to the data frame
 * \param[in] frame_num - the number of the frame
 * 
 */
void LiveViewPlugin::pass_live_frame(boost::shared_ptr<Frame> frame)
{
  void* frame_data_copy = (void*)frame->get_data();

  uint32_t frame_num = frame->get_frame_number();
  std::string aqqID = frame->get_acquisition_id();
  dimensions_t dim = frame->get_dimensions();
  std::string type = get_type_from_enum((DataType)frame->get_data_type());
  std::size_t size = frame->get_data_size();
  std::string compress = get_compress_from_enum((CompressionType)frame->get_compression());
  std::string dataset = frame->get_dataset_name();

  rapidjson::Document document; /* Header info*/
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
  rapidjson::Value valueSize(static_cast<uint64_t>(size));
  document.AddMember(keySize, valueSize, document.GetAllocator());

  //getting dataset
  rapidjson::Value keySet("dataset", document.GetAllocator());
  rapidjson::Value valueSet(dataset.c_str(), document.GetAllocator());
  document.AddMember(keySet, valueSet, document.GetAllocator());

  //getting compression
  rapidjson::Value keyCompress("compression", document.GetAllocator());
  rapidjson::Value valueCompress(compress.c_str(), document.GetAllocator());
  document.AddMember(keyCompress, valueCompress, document.GetAllocator());

  //getting dimensions
  rapidjson::Value keyDims("shape", document.GetAllocator());
  rapidjson::Value valueDims(rapidjson::kArrayType);

  size_t dim_size = dim.size();
  for(size_t i=0; i < dim_size; i++)
  {
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

  time_last_frame_ = boost::posix_time::microsec_clock::local_time();
}

int LiveViewPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int LiveViewPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int LiveViewPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string LiveViewPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string LiveViewPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

/**
 * Sets the config value "per_second" which tells the plugin the number of frames to show each second
 * Setting this value to 0 disables the "frames per second" checking
 * \param[in] value - the value to assign to per_second
 */ 
void LiveViewPlugin::set_per_second_config(int32_t value)
{
  per_second_ = value;

  if(per_second_ == 0)
  {
    LOG4CXX_INFO(logger_, "Disabling Frames Per Second Option");
  }
  else
  {
    LOG4CXX_INFO(logger_, "Displaying " << per_second_ << " frames per second");
    time_between_frames_ = 1000 / per_second_;
  }
}
/**
 * Sets the Frame Frequency config value. This value tells the plugin to show every Nth frame.
 * Setting this value to 0 disables this check
 * \param[in] value - the value to assign to frame_freq
 */ 
void LiveViewPlugin::set_frame_freq_config(int32_t value)
{
  frame_freq_ = value;
  if(frame_freq_ == 0)
  {
    LOG4CXX_INFO(logger_, "Disabling Frame Frequency");
  }
  else
  {
    LOG4CXX_INFO(logger_, "Showing every " << frame_freq_ << " frame(s)");
  }
}
/**
 * Sets the address of the publisher socket that live view data is sent to.
 * When setting this address, it also binds the socket to the address, unbinding it from any previous address given
 * \param[in] value - the address to bind the socket to.
 */
void LiveViewPlugin::set_socket_addr_config(std::string value)
{
  //we dont want to unbind and rebind the same address, as it can cause an error if it takes time to unbind, so we check first
  if(publish_socket_.has_bound_endpoint(value))
  {
    LOG4CXX_WARN(logger_, "Socket already bound to " << value << ". Doing nothing");
    return;
  }
  uint32_t linger = 0;
  publish_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
  publish_socket_.unbind(image_view_socket_addr_.c_str());

  image_view_socket_addr_ = value;
  LOG4CXX_INFO(logger_, "Setting Live View Socket Address to " << image_view_socket_addr_);
  publish_socket_.bind(image_view_socket_addr_);
}

/**
 * Sets the dataset filter configuration. The live view output can be filtered by the dataset variable on the frame based off a list
 * of desired datasets.
 * \param[in] value - A comma deliminated list of dataset names, or an empty string to ignore this filtering.
 */
void LiveViewPlugin::set_dataset_name_config(std::string value)
{
  std::string delim = ",";
  datasets_.clear();
  if(!value.empty())
  {
    //delim value string by comma
    boost::split(datasets_, value, boost::is_any_of(delim));
  }
  std::string dataset_string = "";
  for (int i = 0; i < datasets_.size(); i++)
  {
    boost::trim(datasets_[i]);
    dataset_string += datasets_[i] + ",";
  }
  LOG4CXX_INFO(logger_, "Setting the datasets allowed to: " << dataset_string);
}

}/*namespace FrameProcessor*/



