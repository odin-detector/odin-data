/*
 * DataBlock.h
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_DATABLOCK_H_
#define TOOLS_FILEWRITER_DATABLOCK_H_

#include <stdlib.h>
#include <string.h>

#include <log4cxx/logger.h>

namespace FrameProcessor
{

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

        /** Construct a data block */
        DataBlock(size_t block_size);

        /** Destroy a data block */
        virtual ~DataBlock();

        /** Return the unique index */
        int get_index();

        /** Return the size in bytes */
        size_t get_size();

        /** Copy data from source into block allocated memory */
        void copy_data(const void* data_src, size_t block_size);

        /** Return a const void pointer to memory block owns */
        const void* get_data();

        /** Return a non-const pointer to memory block owns */
        void* get_writeable_data();

        /** Return the current unique index counter */
        static int get_current_index_count();

    private:

        /** Resize the data block */
        void resize(size_t block_size);

        /** Pointer to logger */
        log4cxx::LoggerPtr logger_;

        /** Number of bytes allocated for this DataBlock */
        size_t allocated_bytes_;

        /** Unique index of this DataBlock */
        int index_;

        /** Void pointer to the allocated memory */
        void* block_ptr_;

        /** Static counter for the unique index */
        static int index_counter_;

    };

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_DATABLOCK_H_ */
