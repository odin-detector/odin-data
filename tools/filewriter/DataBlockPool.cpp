/*
 * DataBlockPool.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlockPool.h>

namespace filewriter
{

  std::map<std::string, DataBlockPool*> DataBlockPool::instanceMap_;

  DataBlockPool::~DataBlockPool()
  {
    // TODO Auto-generated destructor stub
  }

  void DataBlockPool::allocate(const std::string& index, size_t nBlocks, size_t nBytes)
  {
    DataBlockPool::instance(index)->internalAllocate(nBlocks, nBytes);
  }

  boost::shared_ptr<DataBlock> DataBlockPool::take(const std::string& index, size_t nBytes)
  {
    return DataBlockPool::instance(index)->internalTake(nBytes);
  }

  void DataBlockPool::release(const std::string& index, boost::shared_ptr<DataBlock> block)
  {
    DataBlockPool::instance(index)->internalRelease(block);
  }

  size_t DataBlockPool::getFreeBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetFreeBlocks();
  }

  size_t DataBlockPool::getUsedBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetUsedBlocks();
  }

  size_t DataBlockPool::getTotalBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetTotalBlocks();
  }

  size_t DataBlockPool::getMemoryAllocated(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetMemoryAllocated();
  }


  DataBlockPool* DataBlockPool::instance(const std::string& index)
  {
     if (DataBlockPool::instanceMap_.count(index) == 0){
       DataBlockPool::instanceMap_[index] = new DataBlockPool();
     }
     return DataBlockPool::instanceMap_[index];
  }

  DataBlockPool::DataBlockPool() :
    freeBlocks_(0),
    usedBlocks_(0),
    totalBlocks_(0),
    memoryAllocated_(0)
  {
  }

  void DataBlockPool::internalAllocate(size_t nBlocks, size_t nBytes)
  {
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);

    // Allocate the number of data blocks, each of size nBytes
    boost::shared_ptr<DataBlock> block;
    for (size_t count = 0; count < nBlocks; count++){
       block = boost::shared_ptr<DataBlock>(new DataBlock(nBytes));
       freeList_.push_front(block);
       // Record the newly allocated block
       freeBlocks_++;
       totalBlocks_++;
       memoryAllocated_ += nBytes;
    }
  }

  boost::shared_ptr<DataBlock> DataBlockPool::internalTake(size_t nBytes)
  {
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);

    boost::shared_ptr<DataBlock> block;
    if (freeBlocks_ == 0){
      if (totalBlocks_ == 0){
        this->internalAllocate(2, nBytes);
      } else {
        this->internalAllocate(totalBlocks_, nBytes);
      }
    }
    if (freeBlocks_ > 0){
      block = freeList_.front();
      if (block->getSize() != nBytes){
        memoryAllocated_ -= block->getSize();
        block->resize(nBytes);
        memoryAllocated_ += nBytes;
      }
      freeList_.pop_front();
      usedMap_[block->getIndex()] = block;
      freeBlocks_--;
      usedBlocks_++;
    }
    return block;
  }

  void DataBlockPool::internalRelease(boost::shared_ptr<DataBlock> block)
  {
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);

    if (usedMap_.count(block->getIndex()) > 0){
      usedMap_.erase(block->getIndex());
    }
    freeList_.push_front(block);
    usedBlocks_--;
    freeBlocks_++;
  }

  size_t DataBlockPool::internalGetFreeBlocks()
  {
    return freeBlocks_;
  }

  size_t DataBlockPool::internalGetUsedBlocks()
  {
    return usedBlocks_;
  }

  size_t DataBlockPool::internalGetTotalBlocks()
  {
    return totalBlocks_;
  }

  size_t DataBlockPool::internalGetMemoryAllocated()
  {
    return memoryAllocated_;
  }

} /* namespace filewriter */
