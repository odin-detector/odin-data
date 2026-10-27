/*
 * UDataBlockPool.h
 *
 *  Created on: 29 September 2026
 *      Author: Famous Alele
 */

#ifndef TOOLS_FILEWRITER_UDATABLOCKPOOL_H_
#define TOOLS_FILEWRITER_UDATABLOCKPOOL_H_

#include <mutex>
#include <unordered_map>

#include <boost/circular_buffer.hpp>

#include "DataBlock.h"

namespace FrameProcessor {

/**
 * The DataBlock and DataBlockPool classes provide memory management for
 * data within Frames. Memory is allocated by a data block on construction,
 * and then the data block can be re-used without continually freeing and re-
 * allocating the memory.
 * The DataBlockPool provides a singleton class that can be used to access
 * data blocks through shared memory pointers and manages the data blocks to
 * avoid continuous allocating and freeing of memory. The DataBlockPool also
 * contains details of how many blocks are available, in use and the total
 * memory used.
 */
class alignas(alignment) UDataBlockPool {

public:
    static constexpr size_t ELEMS_PER_POOL = 64;
    ~UDataBlockPool()
    {
        if (is_pre_allocated_ & (allocated_block_ != nullptr))
            free(allocated_block_);
        allocated_block_ = nullptr;
    }
    UDataBlockPool(UDataBlockPool&&) = delete;
    UDataBlockPool& operator=(UDataBlockPool&&) = delete;
    UDataBlockPool(const volatile UDataBlockPool&) = delete;
    UDataBlockPool& operator=(const volatile UDataBlockPool&) = delete;
    static void* take(size_t block_size);
    static void release(void* block);
    static void tearDownClass();

private:
    static UDataBlockPool* instance(size_t block_size);
    UDataBlockPool(const size_t block_size);
    UDataBlockPool(void* allocated_block_, const size_t block_size, const size_t alignment_offset = 0);
    /**
     * Returns the number of free DataBlocks present in the UDataBlockPool.
     *
     * \return - Number of free DataBlocks.
     */
    size_t instance_get_free_blocks() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return free_list_.size();
    }
    void* internal_take(size_t block_size);
    void internal_release(void* block);
    /**
     * Returns the number of in-use DataBlocks present in the UDataBlockPool.
     *
     * \return - Number of in-use DataBlocks.
     */
    size_t internal_get_used_blocks()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return used_list_.size();
    }
    /**
     * Returns the total number of DataBlocks present in the UDataBlockPool.
     *
     * \return - Total number of DataBlocks.
     */
    size_t internal_get_total_blocks()
    {
        return ELEMS_PER_POOL;
    }
    template <typename T> class PreAllocatedAllocator {
    public:
        using value_type = T;

        PreAllocatedAllocator(T* custom_storage, size_t capacity) :
            storage_(custom_storage),
            capacity_(capacity)
        {
        }

        template <typename U>
        PreAllocatedAllocator(const PreAllocatedAllocator<U>& other) noexcept :
            storage_(reinterpret_cast<T*>(other.storage_)),
            capacity_(other.capacity_)
        {
        }

        T* allocate(std::size_t n)
        {
            if (n > capacity_) {
                throw std::bad_alloc();
            }
            return storage_;
        }

        void deallocate(T* /*p*/, std::size_t /*n*/) noexcept
        {
            // Do nothing: memory lifecycle is managed externally
        }

        template <typename U> struct rebind {
            using other = PreAllocatedAllocator<U>;
        };

        T* storage_;
        std::size_t capacity_;
    };

    /** Mutex used to make this class thread safe */
    mutable std::mutex mutex_;
    // std::atomic<bool> bool_at_;
    PreAllocatedAllocator<void*> free_list_alloc_;
    PreAllocatedAllocator<void*> used_list_alloc_;
    /** List of currently available DataBlock objects */
    boost::circular_buffer<void*, PreAllocatedAllocator<void*>> free_list_;
    /** List of currently in-use DataBlock objects */
    boost::circular_buffer<void*, PreAllocatedAllocator<void*>> used_list_;
    /** pre-allocated memory for the circular buffers */
    alignas(alignment) unsigned char free_list_mem_[(sizeof(void*) * ELEMS_PER_POOL)];
    alignas(alignment) unsigned char used_list_mem_[(sizeof(void*) * ELEMS_PER_POOL)];
    /** Total number of bytes allocated (sum of all DataBlocks) */
    size_t memory_allocated_;
    /** Is pre-allocated  flag */
    bool is_pre_allocated_;
    /** DataBlock header object holding pointer to the allocated contiguous block*/
    alignas(void*) void* allocated_block_;

    // mutex for the static map of pools
    static std::mutex sta_mutex_;
    /** Static map of all UDataBlockPool objects, indexed by their sizes */
    static std::unordered_multimap<size_t, UDataBlockPool*> instance_map_;
    /** Vector for mapping poiners to their allocators */
    static std::vector<std::pair<void*, UDataBlockPool*>> address_mapper_;
};

} /* namespace FrameProcessor */
#endif /* TOOLS_FILEWRITER_UDATABLOCKPOOL_H_ */
