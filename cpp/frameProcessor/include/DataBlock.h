/*
 * DataBlock.h
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_DATABLOCK_H_
#define TOOLS_FILEWRITER_DATABLOCK_H_

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "DebugLevelLogger.h"
#include <log4cxx/logger.h>

namespace FrameProcessor {

/**
 * The DataBlock and DataBlockPool classes provide memory management for
 * data within Frames. Memory is allocated by a data block on construction,
 * and then the data block can be re-used without continually freeing and re-
 * allocating the memory.
 * If a data block is resized then the memory is re-allocated, so data blocks
 * work most efficiently when using the same sized data multiple times. Data
 * can be copied into the allocated block, and a pointer to the raw block is
 * available.
 * Data block memory should NOT be freed outside of the block, when a data block
 * is destroyed it frees its own memory.
 */
class DataBlock {

    friend class DataBlockPool;

public:
    static constexpr int alignment = 64;
    /** Construct a data block */
    DataBlock(size_t block_size) :
        logger_(log4cxx::Logger::getLogger("FP.DataBlock")),
        allocated_bytes_(block_size)
    {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Constructing DataBlock, allocating " << block_size << " bytes");
        // Create this DataBlock's unique index
        index_ = DataBlock::get_static_index_count();
        ++DataBlock::get_static_index_count();
        // Allocate the memory required for this data block
        int rc = posix_memalign(&block_ptr_, alignment, block_size);
        if (rc) {
            LOG4CXX_ERROR(logger_, "Exhausted memory (" << rc << "): could not allocate " << block_size << " bytes");
        }
    }

    /** Destroy a data block */
    ~DataBlock()
    {
        free(block_ptr_);
    }

    /** Return the unique index */
    int get_index() const
    {
        return index_;
    }

    /**
     * Return the size in bytes of this data block.
     *
     * \return - size in bytes of this data block.
     */
    size_t get_size() const
    {
        return allocated_bytes_;
    }

    /**
     * Copy from data source to the allocated memory within this data block.
     * If more bytes are requested to be copied than are available in this
     * block then the copy is truncated to the size of this block.
     *
     * \param[in] data_src - void pointer to the data source.
     * \param[in] block_size - size of data in bytes to copy.
     */
    void copy_data(const void* data_src, size_t block_size)
    {
        if (block_size > allocated_bytes_) {
            LOG4CXX_WARN(
                logger_,
                "Trying to copy: " << block_size << " but allocated buffer only: " << allocated_bytes_
                                   << " bytes. Truncating copy."
            );
            block_size = allocated_bytes_;
        }
        memcpy(block_ptr_, data_src, block_size);
    }

    /**
     * Returns a void pointer to the memory that this data block owns.
     *
     * \return - void pointer to memory owned by this data block.
     */
    const void* get_data() const
    {
        return block_ptr_;
    }

    /**
     * Returns a non-const void pointer to the memory that this data block owns.
     *
     * \return - non-const void pointer to memory owned by this data block
     */
    void* get_writeable_data()
    {
        return block_ptr_;
    }

    /**
     * Returns the current index counter value
     *
     * \return - int current index count
     */
    static int get_current_index_count()
    {
        /** Static counter for the unique index */
        return get_static_index_count();
    }

private:
    /**
     * Resize this data block. The current memory allocation will be
     * freed. Then a new memory block will be allocated to the desired
     * size. If resize is called but the same size is requested, then
     * no actual free or reallocation takes place.
     *
     * \param[in] block_size - new size of this data block.
     */
    void resize(size_t block_size)
    {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Resizing DataBlock " << index_ << " to " << block_size << " bytes");
        // If the new size requested is the different
        // to our current size then re-allocate
        if (block_size != allocated_bytes_) {
            // Free the current allocation first
            free(block_ptr_);
            // Allocate the new number of bytes
            int rc = posix_memalign(&block_ptr_, alignment, block_size);
            if (rc) {
                LOG4CXX_ERROR(
                    logger_, "Exhausted memory (" << rc << "): could not reallocate " << block_size << " bytes"
                );
            }
            // Record our new size
            allocated_bytes_ = block_size;
        }
    }

    /**
     * Returns the current index counter value
     *
     * \return - int current index count
     */
    static int& get_static_index_count()
    {
        /** Static counter for the unique index */
        static int index_counter_ = 0;
        return index_counter_;
    }

    /** Pointer to logger */
    log4cxx::LoggerPtr logger_;

    /** Number of bytes allocated for this DataBlock */
    size_t allocated_bytes_;

    /** Unique index of this DataBlock */
    int index_;

    /** Void pointer to the allocated memory */
    void* block_ptr_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_DATABLOCK_H_ */
