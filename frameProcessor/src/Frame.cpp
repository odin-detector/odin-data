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
    shared_raw_(0),
    shared_size_(0),
    shared_memory_(false),
    shared_id_(0),
    shared_frame_id_(0),
    shared_channel_(0),
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
 * This method sets the dimensions of the frame. The dimensions are
 * stored in a map indexed by a string, which allows several sets of
 * dimensions to be stored (for example, the overall dimensions plus
 * chunked or subframe dimensions).
 *
 * \param[in] type - the string index under which to store these dimensions.
 * \param[in] dimensions - array of dimensions to store.
 */
void Frame::set_dimensions(const std::string& type, const std::vector<unsigned long long>& dimensions)
{
  dimensions_[type] = dimensions;
}

/** Retrieves a particular set of dimension values.
 *
 * This method gets the dimensions of the frame. The dimensions are
 * stored in a map indexed by a string, which allows several sets of
 * dimensions to be stored (for example, the overall dimensions plus
 * chunked or subframe dimensions).
 *
 * \param[in] type - the string index of dimensions to return.
 * \return array of dimensions.
 */
dimensions_t Frame::get_dimensions(const std::string& type) const
{
  return dimensions_.find(type)->second;
}

/** Set a parameter for this Frame.
 *
 * This method sets a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index under which to store the parameter.
 * \param[in] parameter - the parameter to store.
 */
void Frame::set_parameter(const std::string& index, size_t parameter)
{
  parameters_[index] = parameter;
}

/** Return a parameter for this Frame.
 *
 * This method retrieves a parameter of the Frame. The parameters are
 * stored in a map indexed by a string.
 *
 * \param[in] index - the string index of the parameter to return.
 * \return the parameter.
 */
size_t Frame::get_parameter(const std::string& index) const
{
  return parameters_.find(index)->second;
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
