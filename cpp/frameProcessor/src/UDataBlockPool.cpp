/*
 * UDataBlockPool.cpp
 *
 *  Created on: 29 September 2026
 *      Author: Famous Alele
 */

#include "DebugLevelLogger.h"
#include <UDataBlockPool.h>

namespace FrameProcessor {

static constexpr size_t calc_alignment_offset(const size_t block_size)
{
    auto val = (block_size <= alignment) ? (alignment - block_size) : (alignment - (block_size % alignment));
    return val;
}

/**
 * Container of UDataBlockPool instances which can be indexed by name
 */
std::unordered_multimap<size_t, UDataBlockPool*> UDataBlockPool::instance_map_;
std::mutex UDataBlockPool::sta_mutex_;

UDataBlockPool::~UDataBlockPool()
{
}

/**
 * Static method to take a DataBlock from the UDataBlockPool specified by the
 * block_size parameter. New DataBlocks will be allocated if necessary.
 *
 * \param[in] block_size - Size of the DataBlock required in bytes.
 * \return - DataBlock from the available pool, and the UDataBlockPool instance that allocated it.
 */
std::pair<void*, UDataBlockPool*> UDataBlockPool::take(size_t block_size)
{
    auto instance = UDataBlockPool::instance(block_size);
    auto data_blk = instance->internal_take(block_size);
    return { data_blk, instance };
}

/**
 * Static method to release a DataBlock back into the UDataBlockPool specified
 * by the block size. Once a DataBlock has been released it will become
 * available for re-use.
 *
 * \param[in] block - DataBlock to release.
 */
void UDataBlockPool::release(void* block)
{
    // Protect this method
    std::lock_guard<std::mutex> lock(mutex_);

    LOG4CXX_DEBUG_LEVEL(
        2, log4cxx::Logger::getLogger("FP.UDataBlockPool"), "Releasing DataBlock [addr=" << block << "]"
    );

    if (used_map_.count(block) > 0) {
        used_map_.erase(block);
        free_list_.push_front(block);
    }
}

/**
 * Static method that returns the number of in-use DataBlocks present in
 * the UDataBlockPool specified by the block_size parameter.
 *
 * \param[in] block_size - Index of UDataBlockPool to get the in-use count from.
 * \return - Number of in-use DataBlocks.
 */
size_t UDataBlockPool::get_used_blocks(size_t block_size)
{
    return UDataBlockPool::instance(block_size)->internal_get_used_blocks();
}

/**
 * Static method that returns the total number of DataBlocks present in
 * the UDataBlockPool specified by the block_size parameter.
 *
 * \param[in] block_size - Index of UDataBlockPool to get the total count from.
 * \return - Total number of DataBlocks.
 */
size_t UDataBlockPool::get_total_blocks(size_t block_size)
{
    return UDataBlockPool::instance(block_size)->internal_get_total_blocks();
}

/**
 * Static method that returns the total number of bytes that have been
 * allocated by the UDataBlockPool specified by the index parameter.
 *
 * \param[in] index - Index of UDataBlockPool to get the total bytes allocated from.
 * \return - Total number of allocated bytes.
 */
size_t UDataBlockPool::get_memory_allocated(size_t block_size)
{
    auto instance = UDataBlockPool::instance(block_size);
    return instance->internal_get_memory_allocated();
}

/**
 * Static private method that returns a pointer to the UDataBlockPool
 * specified by the index parameter. This is private and is used by
 * all of the static access methods. If no UDataBlockPool exists for
 * the index provided then a new UDataBlockPool is created.
 *
 * \param[in] block_size - Block size of UDataBlockPool to retrieve.
 * \return - Pointer to a UDataBlockPool instance.
 */
UDataBlockPool* UDataBlockPool::instance(size_t block_size)
{
    sta_mutex_.lock();
    auto ranges = UDataBlockPool::instance_map_.equal_range(block_size);
    for (auto it = ranges.first; it != ranges.second; ++it) {
        if (it->second->instance_get_free_blocks()) {
            sta_mutex_.unlock();
            return it->second;
        }
    }
    sta_mutex_.unlock();

    // Allocate new UDataBlockPool
    void* pre_allocated_pool_ptr = nullptr;
    const size_t alignment_offset = calc_alignment_offset(block_size);

    int status = posix_memalign(
        &pre_allocated_pool_ptr, alignment, sizeof(UDataBlockPool) + ((block_size + alignment_offset) * ELEMS_PER_POOL)
    );
    if (!status) {
        new (reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr)) UDataBlockPool(
            reinterpret_cast<uint8_t*>(pre_allocated_pool_ptr) + sizeof(UDataBlockPool), block_size, alignment_offset
        );
        sta_mutex_.lock();
        UDataBlockPool::instance_map_.emplace(block_size, reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr));
        sta_mutex_.unlock();
    } else {
        throw std::runtime_error("Failed to allocate Pool Memory");
    }
    return reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr);
}

/**
 * Construct a UDataBlockPool object. The constructor is private,
 * these pool objects can only be constructed from the static
 * methods.
 */
UDataBlockPool::UDataBlockPool(const size_t block_size) :
    free_list_(ELEMS_PER_POOL)
{
    used_map_.reserve(ELEMS_PER_POOL);
    const size_t alignment_offset = calc_alignment_offset(block_size);
    void* pre_allocated_pool_ptr = nullptr;
    int status = posix_memalign(&pre_allocated_pool_ptr, alignment, (block_size + alignment_offset) * ELEMS_PER_POOL);
    if (!status) {
        alignment_offset_ = alignment_offset;
        memory_allocated_ = (block_size + alignment_offset_) * ELEMS_PER_POOL;
        unsigned char* ptr = reinterpret_cast<unsigned char*>(allocated_block_);
        // initialize the free_list_ pool
        for (int i = 0; i < ELEMS_PER_POOL; ++i) {
            free_list_.push_back(ptr);
            ptr += (block_size + alignment_offset_);
        }
    } else {
        throw std::runtime_error("Failed to allocate Pool Memory");
    }
}

/**
 * Construct a UDataBlockPool object. The constructor is private,
 * these pool objects can only be constructed from the static
 * methods. This constructor take pre-allocated memory
 */
UDataBlockPool::UDataBlockPool(void* allocated_block_ptr, const size_t block_size, const size_t alignment_offset) :
    free_list_(ELEMS_PER_POOL),
    memory_allocated_ { (block_size + alignment_offset) * ELEMS_PER_POOL },
    alignment_offset_ { alignment_offset },
    allocated_block_ { allocated_block_ptr }
{
    used_map_.reserve(ELEMS_PER_POOL);

    unsigned char* ptr = reinterpret_cast<unsigned char*>(allocated_block_);

    // initialize the free_list_ pool with blocks of size block_size
    for (size_t i = 0; i < ELEMS_PER_POOL; ++i) {
        free_list_.push_back(ptr);
        ptr += (block_size + alignment_offset_);
    }
}

/**
 * Take a DataBlock from the UDataBlockPool. New DataBlocks will
 * be allocated if necessary.
 *
 * \param[in] block_size - Size of the DataBlock required in bytes.
 * \return - DataBlock from the available pool.
 */
void* UDataBlockPool::internal_take(size_t block_size)
{
    // Protect this method
    std::lock_guard<std::mutex> lock(mutex_);
    LOG4CXX_DEBUG_LEVEL(
        2, log4cxx::Logger::getLogger("FP.UDataBlockPool"), "Requesting DataBlock of " << block_size << " bytes"
    );
    void* block = free_list_.front();
    free_list_.pop_front();
    used_map_.insert(block);
    LOG4CXX_DEBUG_LEVEL(
        2, log4cxx::Logger::getLogger("FP.UDataBlockPool"), "Providing DataBlock [addr=" << block << "]"
    );
    return block;
}

/**
 * Returns the number of in-use DataBlocks present in the UDataBlockPool.
 *
 * \return - Number of in-use DataBlocks.
 */
size_t UDataBlockPool::internal_get_used_blocks()
{
    return used_map_.size();
}

/**
 * Returns the total number of DataBlocks present in the UDataBlockPool.
 *
 * \return - Total number of DataBlocks.
 */
size_t UDataBlockPool::internal_get_total_blocks()
{
    return ELEMS_PER_POOL;
}

/**
 * Returns the number of bytes allocated by the UDataBlockPool.
 *
 * \return - Number of allocated bytes.
 */
size_t UDataBlockPool::internal_get_memory_allocated()
{
    return memory_allocated_;
}

/**
 * Delete UDataBlockPool instances stored in static class attribute instanceMap_
 */
void UDataBlockPool::tearDownClass()
{
    sta_mutex_.lock();
    for (auto it = instance_map_.begin(); it != instance_map_.end(); it++) {
        delete it->second;
    }
    instance_map_.clear();
    sta_mutex_.unlock();
}

} /* namespace FrameProcessor */
