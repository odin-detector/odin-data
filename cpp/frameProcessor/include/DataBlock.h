/*
 * DataBlock.h
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#ifndef DATABLOCK_H_
#define DATABLOCK_H_

#include <atomic>
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
 * Data block memory allocated in this class should NOT be freed outside of the
 * block, when a data block is destroyed it frees memory it allocated memory.
 */
static constexpr int alignment = 64;
class DataBlock {
    friend class DataBlockPool;
    friend class UDataBlockPool;

public:
    /** DataBlock constructor
     * @param block_size - size of data in bytes
     */
    DataBlock(size_t block_size) :
        allocated_bytes_(block_size),
        is_pre_allocated_ { false }
    {
        LOG4CXX_TRACE(
            log4cxx::Logger::getLogger("FP.DataBlock"), "Constructing DataBlock, allocating " << block_size << " bytes"
        );
        // Create this DataBlock's unique index

        // Allocate the memory required for this data block
        int rc = posix_memalign(&block_ptr_, alignment, block_size);
        if (rc) {
            LOG4CXX_ERROR(
                log4cxx::Logger::getLogger("FP.DataBlock"),
                "Exhausted memory (" << rc << "): could not allocate " << block_size << " bytes"
            );
            throw(std::runtime_error(std::string("DataBlock memory alloc failed (" + block_size) + " bytes)"));
        }
        index_ = DataBlock::counter().fetch_add(1);
    }

    /** DataBlock constructor
     * @param ptr - pointer to pre-allocated memory
     * @param block_size - size of data in bytes
     */
    DataBlock(void* ptr, size_t block_size) noexcept :
        allocated_bytes_ { block_size },
        is_pre_allocated_ { true },
        block_ptr_ { ptr }
    {
        index_ = DataBlock::counter().fetch_add(1);
    }

    /** delete copy constructor! */
    DataBlock(const DataBlock&) = delete;

    /** delete copy assignment operator! */
    DataBlock& operator=(const DataBlock&) = delete;

    /** move constructor */
    DataBlock(DataBlock&& other) noexcept :
        allocated_bytes_ { other.allocated_bytes_ },
        index_ { other.index_ },
        is_pre_allocated_ { other.is_pre_allocated_ },
        block_ptr_ { other.block_ptr_ }
    {
        other.allocated_bytes_ = 0;
        other.index_ = -1;
        other.block_ptr_ = nullptr;
    }

    /** move assignment operator */
    DataBlock& operator=(DataBlock&& other) noexcept
    {
        allocated_bytes_ = other.allocated_bytes_;
        index_ = other.index_;
        block_ptr_ = other.block_ptr_;
        is_pre_allocated_ = other.is_pre_allocated_;
        other.allocated_bytes_ = 0;
        other.index_ = -1;
        other.block_ptr_ = nullptr;
        return *this;
    }

    /** Destroy a data block */
    ~DataBlock() noexcept
    {
        if (!is_pre_allocated_) /** if NOT pre-allocated */
            free(block_ptr_);
    }

    /** Return the unique index */
    int get_index() const noexcept
    {
        return index_;
    }

    /**
     * Return the size in bytes of this data block.
     *
     * \return - size in bytes of this data block.
     */
    size_t get_size() const noexcept
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
                log4cxx::Logger::getLogger("FP.DataBlock"),
                "Trying to copy: " << block_size << " but allocated buffer only: " << allocated_bytes_
                                   << " bytes. Truncating copy."
            );
            block_size = allocated_bytes_;
        }
        memcpy(block_ptr_, data_src, block_size <= allocated_bytes_ ? block_size : allocated_bytes_);
    }

    /**
     * Returns a void pointer to the memory that this data block owns.
     *
     * \return - void pointer to memory owned by this data block.
     */
    const void* get_data() const noexcept
    {
        return block_ptr_;
    }

    /**
     * Returns a non-const void pointer to the memory that this data block owns.
     *
     * \return - non-const void pointer to memory owned by this data block
     */
    void* get_writeable_data() const noexcept
    {
        return block_ptr_;
    }

    /**
     * Returns the current index counter value
     *
     * \return - int current index count
     */
    static int get_current_index_count() noexcept
    {
        /** Static counter for the unique index */
        return counter().load();
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
        LOG4CXX_TRACE(
            log4cxx::Logger::getLogger("FP.DataBlock"),
            "Resizing DataBlock " << index_ << " to " << block_size << " bytes"
        );
        if (is_pre_allocated_) {
            throw std::invalid_argument("Cannot resize fixed buffer!");
        }
        // If the new size requested is the different
        // to our current size then re-allocate
        if (block_size != allocated_bytes_) {
            // Free the current allocation first
            free(block_ptr_);
            // Allocate the new number of bytes
            int rc = posix_memalign(&block_ptr_, alignment, block_size);
            if (rc) {
                LOG4CXX_ERROR(
                    log4cxx::Logger::getLogger("FP.DataBlock"),
                    "Exhausted memory (" << rc << "): could not reallocate " << block_size << " bytes"
                );
                throw(std::runtime_error(std::string("DataBlock memory resize failed (" + block_size) + " bytes)"));
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
    static std::atomic<int>& counter() noexcept
    {
        /** Static counter for the unique index */
        static std::atomic<int> count_ = 0;
        return count_;
    }

    /** Number of bytes allocated for this DataBlock */
    size_t allocated_bytes_;

    /** Unique index of this DataBlock */
    int index_;

    /** is the block pre-allocated */
    bool is_pre_allocated_;

    /** Void pointer to the allocated memory */
    alignas(sizeof(void*)) void* block_ptr_;
};

} /* namespace FrameProcessor */

#endif /* DATABLOCK_H_ */
