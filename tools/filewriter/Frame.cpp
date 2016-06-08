/*
 * NewFrame.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */
#include <Frame.h>

namespace filewriter
{

  Frame::Frame(const std::string& index) :
    logger(log4cxx::Logger::getLogger("Frame"))
  {
    logger->setLevel(log4cxx::Level::getAll());
    LOG4CXX_TRACE(logger, "Frame constructed");
    // Store a default value for the dataset name
    dataset_name = index;
    // Block index is used for taking and releasing data block
    blockIndex_ = index;
  }

  Frame::~Frame()
  {
    LOG4CXX_TRACE(logger, "Frame destroyed");
    // TODO Auto-generated destructor stub
    // If header and raw buffers exist then we must release them
    if (raw_){
      DataBlockPool::release(blockIndex_, raw_);
    }
  }

  void Frame::copy_data(const void* data_src, size_t nbytes)
  {
    LOG4CXX_TRACE(logger, "copy_data called with size: "<< nbytes << " bytes");
    // If we already have a data block then release it
    if (!raw_){
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

  const void* Frame::get_data() const
  {
    if (!raw_){
      throw std::runtime_error("No data allocated in DataBlock");
    }
    return raw_->get_data();
  }

  size_t Frame::get_data_size() const
  {
    if (!raw_){
      throw std::runtime_error("No data allocated in DataBlock");
    }
    return raw_->getSize();
  }

  void Frame::set_dimensions(const std::string& type, const std::vector<unsigned long long>& dimensions)
  {
    dimensions_[type] = dimensions;
  }

  dimensions_t Frame::get_dimensions(const std::string& type) const
  {
    return dimensions_.find(type)->second;
  }

  void Frame::set_parameter(const std::string& index, size_t parameter)
  {
    parameters_[index] = parameter;
  }

  size_t Frame::get_parameter(const std::string& index) const
  {
    return parameters_.find(index)->second;
  }

} /* namespace filewriter */
