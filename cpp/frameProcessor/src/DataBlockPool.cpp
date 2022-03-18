/*
 * DataBlockPool.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlockPool.h>
#include "DebugLevelLogger.h"

namespace FrameProcessor
{

/**
 * Container of DataBlockPool instances which can be indexed by name
 */
std::map<size_t, DataBlockPool*> DataBlockPool::instance_map_;

DataBlockPool::~DataBlockPool()
{
}

/**
 * Static method to force allocation of new DataBlocks which are added to
 * the pool specified by the index parameter.
 *
 * \param[in] block_count - Number of DataBlocks to allocate.
 * \param[in] block_size - Number of bytes to allocate to each block.
 */
void DataBlockPool::allocate(size_t block_count, size_t block_size)
{
  DataBlockPool::instance(block_size)->internal_allocate(block_count, block_size);
}

/**
 * Static method to take a DataBlock from the DataBlockPool specified by the
 * block_size parameter. New DataBlocks will be allocated if necessary.
 *
 * \param[in] block_size - Size of the DataBlock required in bytes.
 * \return - DataBlock from the available pool.
 */
boost::shared_ptr<DataBlock> DataBlockPool::take(size_t block_size)
{
  return DataBlockPool::instance(block_size)->internal_take(block_size);
}

/**
 * Static method to release a DataBlock back into the DataBlockPool specified
 * by the block size. Once a DataBlock has been released it will become
 * available for re-use.
 *
 * \param[in] block - DataBlock to release.
 */
void DataBlockPool::release(boost::shared_ptr<DataBlock> block)
{
  DataBlockPool::instance(block->get_size())->internal_release(block);
}

/**
 * Static method that returns the number of free DataBlocks present in
 * the DataBlockPool specified by the block_size parameter.
 *
 * \param[in] block_size - Index of DataBlockPool to get the free count from.
 * \return - Number of free DataBlocks.
 */
size_t DataBlockPool::get_free_blocks(size_t block_size)
{
  return DataBlockPool::instance(block_size)->internal_get_free_blocks();
}

/**
 * Static method that returns the number of in-use DataBlocks present in
 * the DataBlockPool specified by the block_size parameter.
 *
 * \param[in] block_size - Index of DataBlockPool to get the in-use count from.
 * \return - Number of in-use DataBlocks.
 */
size_t DataBlockPool::get_used_blocks(size_t block_size)
{
  return DataBlockPool::instance(block_size)->internal_get_used_blocks();
}

/**
 * Static method that returns the total number of DataBlocks present in
 * the DataBlockPool specified by the block_size parameter.
 *
 * \param[in] block_size - Index of DataBlockPool to get the total count from.
 * \return - Total number of DataBlocks.
 */
size_t DataBlockPool::get_total_blocks(size_t block_size)
{
  return DataBlockPool::instance(block_size)->internal_get_total_blocks();
}

/**
 * Static method that returns the total number of bytes that have been
 * allocated by the DataBlockPool specified by the index parameter.
 *
 * \param[in] index - Index of DataBlockPool to get the total bytes allocated from.
 * \return - Total number of allocated bytes.
 */
size_t DataBlockPool::get_memory_allocated(size_t block_size)
{
  return DataBlockPool::instance(block_size)->internal_get_memory_allocated();
}

/**
 * Static private method that returns a pointer to the DataBlockPool
 * specified by the index parameter. This is private and is used by
 * all of the static access methods. If no DataBlockPool exists for
 * the index provided then a new DataBlockPool is created.
 *
 * \param[in] block_size - Block size of DataBlockPool to retrieve.
 * \return - Pointer to a DataBlockPool instance.
 */
DataBlockPool* DataBlockPool::instance(size_t block_size)
{
  if (DataBlockPool::instance_map_.count(block_size) == 0) {
    DataBlockPool::instance_map_[block_size] = new DataBlockPool();
  }
  return DataBlockPool::instance_map_[block_size];
}

/**
 * Construct a DataBlockPool object. The constructor is private,
 * these pool objects can only be constructed from the static
 * methods to enforce only one pool for each index is created.
 */
DataBlockPool::DataBlockPool() :
    free_blocks_(0),
    used_blocks_(0),
    total_blocks_(0),
    memory_allocated_(0),
    logger_(log4cxx::Logger::getLogger("FP.DataBlockPool"))
{
}

/**
 * Allocate new DataBlocks to this DataBlockPool. This results in
 * additional memory allocation.
 *
 * \param[in] block_count - Number of DataBlocks to allocate.
 * \param[in] block_size - Number of bytes to allocate to each block.
 */
void DataBlockPool::internal_allocate(size_t block_count, size_t block_size)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Allocating " << block_count <<
                                  " additional DataBlocks of " << block_size << " bytes");

  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Allocate the number of data blocks, each of size block_size
  boost::shared_ptr<DataBlock> block;
  for (size_t count = 0; count < block_count; count++) {
    block = boost::shared_ptr<DataBlock>(new DataBlock(block_size));
    free_list_.push_front(block);
    // Record the newly allocated block
    free_blocks_++;
    total_blocks_++;
    memory_allocated_ += block_size;
  }
}

/**
 * Take a DataBlock from the DataBlockPool. New DataBlocks will
 * be allocated if necessary.
 *
 * \param[in] block_size - Size of the DataBlock required in bytes.
 * \return - DataBlock from the available pool.
 */
boost::shared_ptr<DataBlock> DataBlockPool::internal_take(size_t block_size)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Requesting DataBlock of " << block_size << " bytes");

  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  boost::shared_ptr<DataBlock> block;
  if (free_blocks_ == 0) {
    if (total_blocks_ == 0) {
      this->internal_allocate(2, block_size);
    } else {
      this->internal_allocate(total_blocks_, block_size);
    }
  }
  if (free_blocks_ > 0) {
    block = free_list_.front();
    if (block->get_size() != block_size) {
      memory_allocated_ -= block->get_size();
      block->resize(block_size);
      memory_allocated_ += block_size;
    }
    free_list_.pop_front();
    used_map_[block->get_index()] = block;
    free_blocks_--;
    used_blocks_++;
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Providing DataBlock [id=" << block->get_index() << "]");
  }
  return block;
}

/**
 * Release a DataBlock back into the DataBlockPool. Once a DataBlock has
 * been released it will become available for re-use.
 *
 * \param[in] block - DataBlock to release.
 */
void DataBlockPool::internal_release(boost::shared_ptr<DataBlock> block)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Releasing DataBlock [id=" << block->get_index() << "]");

  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  if (used_map_.count(block->get_index()) > 0) {
    used_map_.erase(block->get_index());
  }
  free_list_.push_front(block);
  used_blocks_--;
  free_blocks_++;
}

/**
 * Returns the number of free DataBlocks present in the DataBlockPool.
 *
 * \return - Number of free DataBlocks.
 */
size_t DataBlockPool::internal_get_free_blocks()
{
  return free_blocks_;
}

/**
 * Returns the number of in-use DataBlocks present in the DataBlockPool.
 *
 * \return - Number of in-use DataBlocks.
 */
size_t DataBlockPool::internal_get_used_blocks()
{
  return used_blocks_;
}

/**
 * Returns the total number of DataBlocks present in the DataBlockPool.
 *
 * \return - Total number of DataBlocks.
 */
size_t DataBlockPool::internal_get_total_blocks()
{
  return total_blocks_;
}

/**
 * Returns the number of bytes allocated by the DataBlockPool.
 *
 * \return - Number of allocated bytes.
 */
size_t DataBlockPool::internal_get_memory_allocated()
{
  return memory_allocated_;
}

/**
 * Delete DataBlockPool instances stored in static class attribute instanceMap_
 */
void DataBlockPool::tearDownClass()
{
  std::map<size_t , DataBlockPool*>::iterator it;
  for (it = instance_map_.begin(); it != instance_map_.end(); it++) {
    delete it->second;
  }
  instance_map_.clear();
}

} /* namespace FrameProcessor */
