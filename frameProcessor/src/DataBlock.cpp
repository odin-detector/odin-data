/*
 * DataBlock.cpp
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#include <DataBlock.h>
#include "DebugLevelLogger.h"
#include <malloc.h>

namespace FrameProcessor
{
int DataBlock::index_counter_ = 0;

static const int alignment = 64;

/**
 * Construct a data block, allocating the required memory.
 *
 * \param[in] block_size - number of bytes to allocate.
 */
DataBlock::DataBlock(size_t block_size) :
    logger_(log4cxx::Logger::getLogger("FP.DataBlock")),
    allocated_bytes_(block_size)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Constructing DataBlock, allocating " << block_size << " bytes");
  // Create this DataBlock's unique index
  index_ = DataBlock::index_counter_++;
  // Allocate the memory required for this data block
  block_ptr_ = memalign(alignment, block_size);
}

/**
 * Destroy a data block, freeing resources.
 */
DataBlock::~DataBlock()
{
  // Free the memory
  free(block_ptr_);
}

/**
 * Return the unique index of this data block.
 *
 * \return - unique index of this data block.
 */
int DataBlock::get_index()
{
  return index_;
}

/**
 * Return the size in bytes of this data block.
 *
 * \return - size in bytes of this data block.
 */
size_t DataBlock::get_size()
{
  return allocated_bytes_;
}

/**
 * Resize this data block. The current memory allocation will be
 * freed. Then a new memory block will be allocated to the desired
 * size. If resize is called but the same size is requested, then
 * no actual free or reallocation takes place.
 *
 * \param[in] block_size - new size of this data block.
 */
void DataBlock::resize(size_t block_size)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Resizing DataBlock " << index_ << " to " << block_size << " bytes");
  // If the new size requested is the different
  // to our current size then re-allocate
  if (block_size != allocated_bytes_) {
    // Free the current allocation first
    free(block_ptr_);
    // Allocate the new number of bytes
    block_ptr_ = memalign(alignment, block_size);
    // Record our new size
    allocated_bytes_ = block_size;
  }
}

/**
 * Copy from data source to the allocated memory within this data block.
 * If more bytes are requested to be copied than are available in this
 * block then the copy is truncated to the size of this block.
 *
 * \param[in] data_src - void pointer to the data source.
 * \param[in] block_size - size of data in bytes to copy.
 */
void DataBlock::copy_data(const void* data_src, size_t block_size)
{
  if (block_size > allocated_bytes_) {
    LOG4CXX_WARN(logger_, "Trying to copy: " << block_size <<
                          " but allocated buffer only: " << allocated_bytes_ <<
                          " bytes. Truncating copy.");
    block_size = allocated_bytes_;
  }
  memcpy(block_ptr_, data_src, block_size);
}

/**
 * Returns a void pointer to the memory that this data block owns.
 *
 * \return - void pointer to memory owned by this data block.
 */
const void* DataBlock::get_data()
{
  return block_ptr_;
}

/**
 * Returns a non-const void pointer to the memory that this data block owns.
 *
 * \return - non-const void pointer to memory owned by this data block
 */
void* DataBlock::get_writeable_data()
{
  return block_ptr_;
}

/**
 * Returns the current index counter value
 *
 * \return - int current index count
 */
int DataBlock::get_current_index_count() {
  return DataBlock::index_counter_;
}


} /* namespace FrameProcessor */
