/*
 * UDataBlockPool.cpp
 *
 *  Created on: 29 September 2026
 *      Author: Famous Alele
 */

/**
 * Some Notes:
 * UDataBlockPool::instance(block_size)->UDataBlockPool_Method();
 *                                      ^
 *                                      |
 * -------------------------------------|
 * This is not an atomic operation, an atomic test is necessary and the process returns
 * if it fails!
 */
#include "UDataBlockPool.h"
#include "DebugLevelLogger.h"
#include <algorithm>

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
std::vector<std::pair<void*, UDataBlockPool*>> UDataBlockPool::address_mapper_;
std::mutex UDataBlockPool::sta_mutex_;

/**
 * Static method to take a DataBlock from the UDataBlockPool specified by the
 * block_size parameter. New DataBlocks will be allocated if necessary.
 *
 * \param[in] block_size - Size of the DataBlock required in bytes.
 * \return - DataBlock from the available pool, and the UDataBlockPool instance that allocated it.
 */
void* UDataBlockPool::take(size_t block_size)
{
    // handle nullptr case!
    return UDataBlockPool::instance(block_size)->internal_take(block_size);
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
    std::unique_lock<std::mutex> lock(sta_mutex_);

    LOG4CXX_DEBUG_LEVEL(
        2, log4cxx::Logger::getLogger("FP.UDataBlockPool"), "Releasing DataBlock [addr=" << block << "]"
    );

    UDataBlockPool* pool
        = upper_bound(address_mapper_.cbegin(), address_mapper_.cend(), block, [](void* block, auto& item) {
              return block < item.first;
          })->second;
    lock.unlock();
    pool->internal_release(block);
}

void UDataBlockPool::internal_release(void* block)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (used_map_.count(block) > 0) {
        used_map_.erase(block);
        free_list_.push_front(block);
    }
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
        void* last_memory_addr = reinterpret_cast<uint8_t*>(pre_allocated_pool_ptr)
            + (sizeof(UDataBlockPool) + ((block_size + alignment_offset) * ELEMS_PER_POOL));
        new (reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr)) UDataBlockPool(
            reinterpret_cast<uint8_t*>(pre_allocated_pool_ptr) + sizeof(UDataBlockPool), block_size, alignment_offset
        );
        sta_mutex_.lock();
        UDataBlockPool::instance_map_.emplace(block_size, reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr));
        UDataBlockPool::address_mapper_.push_back(
            std::pair { last_memory_addr, reinterpret_cast<UDataBlockPool*>(pre_allocated_pool_ptr) }
        );
        std::sort(address_mapper_.begin(), address_mapper_.end(), [](auto& a, auto& b) { return a.first < b.first; });
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
    if (free_list_.empty())
        return nullptr;
    void* block = free_list_.front();
    free_list_.pop_front();
    used_map_.insert(block);
    LOG4CXX_DEBUG_LEVEL(
        2, log4cxx::Logger::getLogger("FP.UDataBlockPool"), "Providing DataBlock [addr=" << block << "]"
    );
    return block;
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
