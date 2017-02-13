/*
 * DataBlockPool.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlockPool.h>

namespace filewriter
{

  /**
   * Container of DataBlockPool instances which can be indexed by name
   */
  std::map<std::string, DataBlockPool*> DataBlockPool::instanceMap_;

  DataBlockPool::~DataBlockPool()
  {
  }

  /**
   * Static method to force allocation of new DataBlocks which are added to
   * the pool specified by the index parameter.
   *
   * \param[in] index - Index of DataBlockPool to add new DataBlocks to.
   * \param[in] nBlocks - Number of DataBlocks to allocate.
   * \param[in] nBytes - Number of bytes to allocate to each block.
   */
  void DataBlockPool::allocate(const std::string& index, size_t nBlocks, size_t nBytes)
  {
    DataBlockPool::instance(index)->internalAllocate(nBlocks, nBytes);
  }

  /**
   * Static method to take a DataBlock from the DataBlockPool specified by the
   * index parameter.  New DataBlocks will be allocated if necessary.
   *
   * \param[in] index - Index of DataBlockPool to take the DataBlocks from.
   * \param[in] nBytes - Size of the DataBlock required in bytes.
   * \return - DataBlock from the available pool.
   */
  boost::shared_ptr<DataBlock> DataBlockPool::take(const std::string& index, size_t nBytes)
  {
    return DataBlockPool::instance(index)->internalTake(nBytes);
  }

  /**
   * Static method to release a DataBlock back into the DataBlockPool specified
   * by the index parameter.  Once a DataBlock has been released it will become
   * available for re-use.
   *
   * \param[in] index - Index of DataBlockPool to take the DataBlocks from.
   * \param[in] block - DataBlock to release.
   */
  void DataBlockPool::release(const std::string& index, boost::shared_ptr<DataBlock> block)
  {
    DataBlockPool::instance(index)->internalRelease(block);
  }

  /**
   * Static method that returns the number of free DataBlocks present in
   * the DataBlockPool specified by the index parameter.
   *
   * \param[in] index - Index of DataBlockPool to get the free count from.
   * \return - Number of free DataBlocks.
   */
  size_t DataBlockPool::getFreeBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetFreeBlocks();
  }

  /**
   * Static method that returns the number of in-use DataBlocks present in
   * the DataBlockPool specified by the index parameter.
   *
   * \param[in] index - Index of DataBlockPool to get the in-use count from.
   * \return - Number of in-use DataBlocks.
   */
  size_t DataBlockPool::getUsedBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetUsedBlocks();
  }

  /**
   * Static method that returns the total number of DataBlocks present in
   * the DataBlockPool specified by the index parameter.
   *
   * \param[in] index - Index of DataBlockPool to get the total count from.
   * \return - Total number of DataBlocks.
   */
  size_t DataBlockPool::getTotalBlocks(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetTotalBlocks();
  }

  /**
   * Static method that returns the total number of bytes that have been
   * allocated by the DataBlockPool specified by the index parameter.
   *
   * \param[in] index - Index of DataBlockPool to get the total bytes allocated from.
   * \return - Total number of allocated bytes.
   */
  size_t DataBlockPool::getMemoryAllocated(const std::string& index)
  {
    return DataBlockPool::instance(index)->internalGetMemoryAllocated();
  }

  /**
   * Static private method that returns a pointer to the DataBlockPool
   * specified by the index parameter.  This is private and is used by
   * all of the static access methods.  If no DataBlockPool exists for
   * the index provided then a new DataBlockPool is created.
   *
   * \param[in] index - Index of DataBlockPool to retrieve.
   * \return - Pointer to a DataBlockPool instance.
   */
  DataBlockPool* DataBlockPool::instance(const std::string& index)
  {
     if (DataBlockPool::instanceMap_.count(index) == 0){
       DataBlockPool::instanceMap_[index] = new DataBlockPool();
     }
     return DataBlockPool::instanceMap_[index];
  }

  /**
   * Construct a DataBlockPool object.  The constructor is private,
   * these pool objects can only be constructed from the static
   * methods to enforce only one pool for each index is created.
   */
  DataBlockPool::DataBlockPool() :
    freeBlocks_(0),
    usedBlocks_(0),
    totalBlocks_(0),
    memoryAllocated_(0),
    logger_(log4cxx::Logger::getLogger("FW.DataBlockPool"))
  {
  }

  /**
   * Allocate new DataBlocks to this DataBlockPool.  This results in
   * additional memory allocation.
   *
   * \param[in] nBlocks - Number of DataBlocks to allocate.
   * \param[in] nBytes - Number of bytes to allocate to each block.
   */
  void DataBlockPool::internalAllocate(size_t nBlocks, size_t nBytes)
  {
    LOG4CXX_DEBUG(logger_, "Allocating " << nBlocks << " additional DataBlocks of " << nBytes << " bytes");

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

  /**
   * Take a DataBlock from the DataBlockPool.  New DataBlocks will
   * be allocated if necessary.
   *
   * \param[in] nBytes - Size of the DataBlock required in bytes.
   * \return - DataBlock from the available pool.
   */
  boost::shared_ptr<DataBlock> DataBlockPool::internalTake(size_t nBytes)
  {
    LOG4CXX_DEBUG(logger_, "Requesting DataBlock of " << nBytes << " bytes");

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
      LOG4CXX_DEBUG(logger_, "Providing DataBlock [id=" << block->getIndex() << "]");
    }
    return block;
  }

  /**
   * Release a DataBlock back into the DataBlockPool.  Once a DataBlock has
   * been released it will become available for re-use.
   *
   * \param[in] block - DataBlock to release.
   */
  void DataBlockPool::internalRelease(boost::shared_ptr<DataBlock> block)
  {
    LOG4CXX_DEBUG(logger_, "Releasing DataBlock [id=" << block->getIndex() << "]");

    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);

    if (usedMap_.count(block->getIndex()) > 0){
      usedMap_.erase(block->getIndex());
    }
    freeList_.push_front(block);
    usedBlocks_--;
    freeBlocks_++;
  }

  /**
   * Returns the number of free DataBlocks present in the DataBlockPool.
   *
   * \return - Number of free DataBlocks.
   */
  size_t DataBlockPool::internalGetFreeBlocks()
  {
    return freeBlocks_;
  }

  /**
   * Returns the number of in-use DataBlocks present in the DataBlockPool.
   *
   * \return - Number of in-use DataBlocks.
   */
  size_t DataBlockPool::internalGetUsedBlocks()
  {
    return usedBlocks_;
  }

  /**
   * Returns the total number of DataBlocks present in the DataBlockPool.
   *
   * \return - Total number of DataBlocks.
   */
  size_t DataBlockPool::internalGetTotalBlocks()
  {
    return totalBlocks_;
  }

  /**
   * Returns the number of bytes allocated by the DataBlockPool.
   *
   * \return - Number of allocated bytes.
   */
  size_t DataBlockPool::internalGetMemoryAllocated()
  {
    return memoryAllocated_;
  }

} /* namespace filewriter */
