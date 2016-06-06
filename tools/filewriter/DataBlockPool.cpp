/*
 * DataBlockPool.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlockPool.h>

namespace filewriter
{

  DataBlockPool *DataBlockPool::instance_;

  DataBlockPool::~DataBlockPool()
  {
    // TODO Auto-generated destructor stub
  }

  void DataBlockPool::allocate(size_t nBlocks, size_t nBytes)
  {
    DataBlockPool::instance()->internalAllocate(nBlocks, nBytes);
  }

  boost::shared_ptr<DataBlock> DataBlockPool::take(size_t nBytes)
  {
    return DataBlockPool::instance()->internalTake(nBytes);
  }

  void DataBlockPool::release(boost::shared_ptr<DataBlock> block)
  {
    DataBlockPool::instance()->internalRelease(block);
  }

  size_t DataBlockPool::getFreeBlocks()
  {
    return DataBlockPool::instance()->internalGetFreeBlocks();
  }

  size_t DataBlockPool::getUsedBlocks()
  {
    return DataBlockPool::instance()->internalGetUsedBlocks();
  }

  size_t DataBlockPool::getTotalBlocks()
  {
    return DataBlockPool::instance()->internalGetTotalBlocks();
  }

  size_t DataBlockPool::getMemoryAllocated()
  {
    return DataBlockPool::instance()->internalGetMemoryAllocated();
  }


  DataBlockPool* DataBlockPool::instance()
  {
     if (!DataBlockPool::instance_){
       DataBlockPool::instance_ = new DataBlockPool();
     }
     return DataBlockPool::instance_;
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
    boost::shared_ptr<DataBlock> block;
    // Allocate the number of data blocks, each of size nBytes
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
    boost::shared_ptr<DataBlock> block;
    // TODO: Allocate more blocks
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
