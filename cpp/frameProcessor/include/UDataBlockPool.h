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
#include <boost/unordered/unordered_flat_set.hpp>

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
    ~UDataBlockPool() = default;

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
        return free_list_.size();
    }
    void internal_allocate(size_t block_count, size_t block_size);
    void* internal_take(size_t block_size);
    void internal_release(void* block);
    /**
     * Returns the number of in-use DataBlocks present in the UDataBlockPool.
     *
     * \return - Number of in-use DataBlocks.
     */
    size_t internal_get_used_blocks()
    {
        return used_map_.size();
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

    /** Mutex used to make this class thread safe */
    std::mutex mutex_;
    // std::atomic<bool> bool_at_;
    /** List of currently available DataBlock objects */
    boost::circular_buffer<void*> free_list_;
    // unsigned char free_list_mem_[(sizeof(DataBlock*) * ELEMS_PER_POOL)];
    /** Map of currently used DataBlock objects, indexed by their unique IDs */
    boost::unordered::unordered_flat_set<void*> used_map_;
    // unsigned char used_map_mem_[((sizeof(std::pair<int, DataBlock*>) * ELEMS_PER_POOL)) + 32];
    /** Total number of bytes allocated (sum of all DataBlocks) */
    size_t memory_allocated_;
    /** Offset size for block alignment */
    size_t alignment_offset_;
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
