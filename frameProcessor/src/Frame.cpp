/*
 * Frame.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#include <Frame.h>

namespace FrameProcessor
{

/** Construct a new Frame object.
 *
 * The constructor sets up logging used within the class, and records
 * the supplied index.
 */
Frame::Frame(const std::string& index) :
    bytes_per_pixel(0),
    frameNumber_(0),
    frameOffset_(0),
    shared_raw_(0),
    shared_size_(0),
    shared_memory_(false),
    shared_id_(0),
    shared_frame_id_(0),
    shared_channel_(0),
    data_type_(-1),
    compression_(-1),
    logger(log4cxx::Logger::getLogger("FW.Frame"))
{
  logger->setLevel(log4cxx::Level::getAll());
  LOG4CXX_TRACE(logger, "Frame constructed");
  // Store a default value for the dataset name
  dataset_name = index;
  // Block index is used for taking and releasing data block
  blockIndex_ = index;
}

/** Destructor
 *
 * The destructor releases any allocated memory back into the pool.
 */
Frame::~Frame()
{
  LOG4CXX_TRACE(logger, "Frame destroyed");
  if (shared_memory_) {
    // We must notify the frame receiver that the buffer is now available
    // We want to send the notify frame release message back to the frameReceiver
    OdinData::IpcMessage txMsg(OdinData::IpcMessage::MsgTypeNotify,
                               OdinData::IpcMessage::MsgValNotifyFrameRelease);
    txMsg.set_param("frame", shared_frame_id_);
    txMsg.set_param("buffer_id", shared_id_);
    LOG4CXX_DEBUG(logger, "Sending response: " << txMsg.encode());

    // Now publish the release message, to notify the frame receiver that we are
    // finished with that block of shared memory
    shared_channel_->send(txMsg.encode());
  } else {
    // If header and raw buffers exist then we must release them
    if (raw_) {
      DataBlockPool::release(blockIndex_, raw_);
    }
  }
}

/** Wrap this frame around an existing shared memory buffer.
 *
 * This method does not need to perform any copying of memory.
 * It simply wraps around the shared buffer.
 *
 * \param[in] data_src - pointer to the raw data of the shared memory buffer.
 * \param[in] nbytes - size of the shared memory buffer.
 * \param[in] bufferID - ID of the shared memory buffer.
 */
void Frame::wrap_shared_buffer(const void* data_src,
                               size_t nbytes,
                               uint64_t bufferID,
                               int frameID,
                               OdinData::IpcChannel *relCh)
{
  shared_raw_ = data_src;
  shared_size_ = nbytes;
  shared_id_ = bufferID;
  shared_channel_ = relCh;
  shared_frame_id_ = frameID;
  shared_memory_ = true;
}

/** Copy raw data into the frame's data block.
 *
 * This method obtains a DataBlock from the DataBlockPool of the correct size.
 * It then copies the source data into the DataBlock.
 *
 * \param[in] data_src - pointer to the raw data to copy into this frame.
 * \param[in] nbytes - number of bytes to copy.
 */
void Frame::copy_data(const void* data_src, size_t nbytes)
{
  LOG4CXX_TRACE(logger, "copy_data called with size: " << nbytes << " bytes");
  if (!shared_memory_) {
    // If we already have a data block then release it
    if (!raw_) {
      // Take a new data block from the pool
      raw_ = DataBlockPool::take(blockIndex_, nbytes);
    } else {
      LOG4CXX_TRACE(logger, "Data block already exists");
      DataBlockPool::release(blockIndex_, raw_);
      raw_ = DataBlockPool::take(blockIndex_, nbytes);
    }
    // Copy the data into the DataBlock
    raw_->copyData(data_src, nbytes);
  }
}

/** Return a void pointer to the raw data.
 *
 * Check to see that this Frame owns a DataBlock, and then return
 * the pointer to the raw data.
 *
 * \return pointer to the raw data.
 */
const void* Frame::get_data() const
{
  const void *rPtr = 0;
  if (shared_memory_) {
    rPtr = shared_raw_;
  } else {
    if (!raw_) {
      throw std::runtime_error("No data allocated in DataBlock");
    }
    rPtr = raw_->get_data();
  }
  return rPtr;
}

/** Returns the size of the raw data.
 *
 * \return the size of the raw data in bytes.
 */
size_t Frame::get_data_size() const
{
  size_t size = 0;
  if (shared_memory_) {
    size = shared_size_;
  } else {
    if (!raw_) {
      throw std::runtime_error("No data allocated in DataBlock");
    }
    size = raw_->getSize();
  }
  return size;
}

/** Sets a particular set of dimension values.
 *
 * This method sets the dimensions of the frame.
 *
 * \param[in] dimensions - array of dimensions to store.
 */
void Frame::set_dimensions(const std::vector<unsigned long long>& dimensions)
{
  dimensions_ = dimensions;
}

/** Retrieves a particular set of dimension values.
 *
 * This method gets the dimensions of the frame.
 *
 * \return array of dimensions.
 */
dimensions_t Frame::get_dimensions() const
{
  return dimensions_;
}

/** Retrieves the compression type of the raw data.
 *
 * -1: Unset, 0: None, 1: LZ4, 2: BSLZ4
 *
 * \return compression type.
 */
int Frame::get_compression() const
{
  return compression_;
}

/** Retrieves the data type of the raw data.
 *
 * -1: Unset, 0: UINT8, 1: UINT16, 2: UINT32
 *
 * \return data type.
 */
int Frame::get_data_type() const
{
  return data_type_;
}

/** Set a uint8_t parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter value to store.
 */
void Frame::set_parameter(const std::string& index, uint8_t value)
{
  Parameter param;
  param.type = raw_8bit;
  param.value.i8_val = value;
  parameters_[index] = param;
}

/** Set a uint16_t parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter value to store.
 */
void Frame::set_parameter(const std::string& index, uint16_t value)
{
  Parameter param;
  param.type = raw_16bit;
  param.value.i16_val = value;
  parameters_[index] = param;
}

/** Set a uint32_t parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter value to store.
 */
void Frame::set_parameter(const std::string& index, uint32_t value)
{
  Parameter param;
  param.type = raw_32bit;
  param.value.i32_val = value;
  parameters_[index] = param;
}

/** Set a uint64_t parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter value to store.
 */
void Frame::set_parameter(const std::string& index, uint64_t value)
{
  Parameter param;
  param.type = raw_64bit;
  param.value.i64_val = value;
  parameters_[index] = param;
}

/** Set a float parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter value to store.
 */
void Frame::set_parameter(const std::string& index, float value)
{
  Parameter param;
  param.type = raw_float;
  param.value.float_val = value;
  parameters_[index] = param;
}

/** Set the compression type of the raw data.
 *
 * 0: None, 1: LZ4, 2: BSLZ4
 *
 * \param compression type.
 */
void Frame::set_compression(int compression)
{
  compression_ = compression;
}

/** Set the data type of the raw data.
 *
 * 0: UINT8, 1: UINT16, 2: UINT32
 *
 * \param data type.
 */
void Frame::set_data_type(int data_type)
{
  data_type_ = data_type;
}

/** Return a parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter.
 */
Parameter Frame::get_parameter(const std::string& index) const
{
  std::map<std::string, Parameter>::const_iterator iter = parameters_.find(index);
  if (iter == parameters_.end())
  {
    LOG4CXX_ERROR(logger, "Unable to find parameter: " << index);
    throw std::runtime_error("Unable to find parameter:");
  }
  return iter->second;
}

/** Return the value of a uint8_t parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter value.
 */
uint8_t Frame::get_i8_parameter(const std::string& index) const
{
  Parameter param = get_parameter(index);
  if (param.type != raw_8bit)
  {
    LOG4CXX_ERROR(logger, "Parameter: " << index << " has wrong type");
    throw std::runtime_error("Parameter has wrong type");
  }

  return param.value.i8_val;
}

/** Return the value of a uint16_t parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter value.
 */
uint16_t Frame::get_i16_parameter(const std::string& index) const
{
  Parameter parameter = get_parameter(index);
  if (parameter.type != raw_16bit)
  {
    LOG4CXX_ERROR(logger, "Parameter: " << index << " has wrong type");
    throw std::runtime_error("Parameter has wrong type");
  }

  return parameter.value.i16_val;
}

/** Return the value of a uint32_t parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter value.
 */
uint32_t Frame::get_i32_parameter(const std::string& index) const
{
  Parameter parameter = get_parameter(index);
  if (parameter.type != raw_32bit)
  {
    LOG4CXX_ERROR(logger, "Parameter: " << index << " has wrong type");
    throw std::runtime_error("Parameter has wrong type");
  }

  return parameter.value.i32_val;
}

/** Return the value of a uint64_t parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter value.
 */
uint64_t Frame::get_i64_parameter(const std::string& index) const
{
  Parameter parameter = get_parameter(index);
  if (parameter.type != raw_64bit)
  {
    LOG4CXX_ERROR(logger, "Parameter: " << index << " has wrong type");
    throw std::runtime_error("Parameter has wrong type");
  }

  return parameter.value.i64_val;
}

/** Return the value of a float parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter value.
 */
float Frame::get_float_parameter(const std::string& index) const
{
  Parameter parameter = get_parameter(index);
  if (parameter.type != raw_float)
  {
    LOG4CXX_ERROR(logger, "Parameter: " << index << " has wrong type");
    throw std::runtime_error("Parameter has wrong type");
  }

  return parameter.value.float_val;
}

/** Return the parameters for this Frame.
 *
 * This method retrieves the map of Parameters
 *
 * \return the parameters.
 */
std::map<std::string, Parameter> & Frame::get_parameters()
{
  return parameters_;
}

/** Check if the Frame contains a parameter.
 *
 * This method checks if the parameter exists within the Frame.
 *
 * \param[in] index - the string index of the parameter to check.
 * \return true if the parameter exists, false otherwise.
 */
bool Frame::has_parameter(const std::string& index)
{
  return (parameters_.count(index) == 1);
}

} /* namespace FrameProcessor */
