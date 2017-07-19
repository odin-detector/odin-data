/*
 * DataBlock.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlock.h>

namespace FrameProcessor
{
int DataBlock::indexCounter_ = 0;


/**
 * Construct a data block, allocating the required memory.
 *
 * \param[in] nbytes - number of bytes to allocate.
 */
DataBlock::DataBlock(size_t nbytes) :
    logger_(log4cxx::Logger::getLogger("FP.DataBlock")),
    allocatedBytes_(nbytes)
{
  LOG4CXX_DEBUG(logger_, "Constructing DataBlock, allocating " << nbytes << " bytes");
  // Create this DataBlock's unique index
  index_ = DataBlock::indexCounter_++;
  // Allocate the memory required for this data block
  blockPtr_ = malloc(nbytes);
}

/**
 * Destroy a data block, freeing resources.
 */
DataBlock::~DataBlock()
{
  // Free the memory
  free(blockPtr_);
}

/**
 * Return the unique index of this data block.
 *
 * \return - unique index of this data block.
 */
int DataBlock::getIndex()
{
  // Return the index of this block
  return index_;
}

/**
 * Return the size in bytes of this data block.
 *
 * \return - size in bytes of this data block.
 */
size_t DataBlock::getSize()
{
  // Just return the current size
  return allocatedBytes_;
}

/**
 * Resize this data block. The current memory allocation will be
 * freed. Then a new memory block will be allocated to the desired
 * size. If resize is called but the same size is requested, then
 * no actual free or reallocation takes place.
 *
 * \param[in] nbytes - new size of this data block.
 */
void DataBlock::resize(size_t nbytes)
{
  LOG4CXX_DEBUG(logger_, "Resizing DataBlock " << index_ << " to " << nbytes << " bytes");
  // If the new size requested is the different
  // to our current size then re-allocate
  if (nbytes != allocatedBytes_) {
    // Free the current allocation first
    free(blockPtr_);
    // Allocate the new number of bytes
    blockPtr_ = malloc(nbytes);
    // Record our new size
    allocatedBytes_ = nbytes;
  }
}

/**
 * Copy from data source to the allocated memory within this data block.
 * If more bytes are requested to be copied than are available in this
 * block then the copy is truncated to the size of this block.
 *
 * \param[in] data_src - void pointer to the data source.
 * \param[in] nbytes - size of data in bytes to copy.
 */
void DataBlock::copyData(const void* data_src, size_t nbytes)
{
  if (nbytes > allocatedBytes_) {
    LOG4CXX_WARN(logger_, "Trying to copy: " << nbytes <<
                                             " but allocated buffer only: " << allocatedBytes_ <<
                                             " bytes. Truncating copy.");
    nbytes = allocatedBytes_;
  }
  memcpy(blockPtr_, data_src, nbytes);
}

/**
 * Returns a void pointer to the memory that this data block owns.
 *
 * \return - void pointer to memory owned by this data block.
 */
const void* DataBlock::get_data()
{
  // This returns the pointer to the data block
  return blockPtr_;
}

} /* namespace FrameProcessor */
