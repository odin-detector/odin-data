#ifndef DATABLOCKFRAME_H
#define DATABLOCKFRAME_H

#include "DataBlock.h"
#include "Frame.h"
#include "UDataBlockPool.h"

namespace FrameProcessor {

class DataBlockFrame : public Frame {

public:
    /** DataBlockFrame constructor
     *
     * @tparam Q - Template parameter to resolve to FrameMetaData
     * @param meta_data - frame FrameMetaData
     * @param data_src - data to copy into block memory
     * @param block_size - size of data in bytes
     * @param image_offset - between start of data memory and image
     */
    template <
        typename Q,
        typename = typename std::enable_if<std::is_same<typename std::decay<Q>::type, FrameMetaData>::value>::type>
    DataBlockFrame(Q&& meta_data, const void* data_src, size_t block_size, const int image_offset = 0) :
        Frame(std::forward<Q>(meta_data), block_size, image_offset),
        is_preallocated_ { false },
        data_block_ { nullptr, 0 }
    {
        void* ptr = UDataBlockPool::take(block_size);
        data_block_ = DataBlock(ptr, block_size);
        data_block_.copy_data(data_src, block_size);
    }

    /** DataBlockFrame constructor
     *
     * @tparam Q - Template parameter to resolve to FrameMetaData
     * @param block_ptr - address of preallocated block memory for the DataBlock
     * @param data_src - data to copy into block memory
     * @param block_size - size of data in bytes of the preallocated block memory
     * @param meta_data - frame's FrameMetaData
     * @param image_offset - between start of data memory and image
     */
    template <
        typename Q,
        typename = typename std::enable_if<std::is_same<typename std::decay<Q>::type, FrameMetaData>::value>::type>
    DataBlockFrame(
        void* block_ptr,
        const void* data_src,
        size_t block_size,
        Q&& meta_data,
        const int image_offset = 0
    ) :
        Frame(std::forward<Q>(meta_data), block_size, image_offset),
        is_preallocated_ { true },
        data_block_ { block_ptr, block_size }
    {
        data_block_.copy_data(data_src, block_size);
    }

    /** DataBlockFrame constructor
     * @tparam Q - Template parameter to resolve to FrameMetaData
     * @param meta_data - frame FrameMetaData
     * @param block_size - size of data in bytes
     * @param image_offset - between start of data memory and image
     */
    template <
        typename Q,
        typename = typename std::enable_if<std::is_same<typename std::decay<Q>::type, FrameMetaData>::value>::type>
    DataBlockFrame(Q&& meta_data, size_t block_size, const int image_offset = 0) :
        Frame(std::forward<Q>(meta_data), block_size, image_offset),
        is_preallocated_ { false },
        data_block_ { nullptr, 0 }

    {
        void* ptr = UDataBlockPool::take(block_size);
        data_block_ = DataBlock(ptr, block_size);
    }

    /** DataBlockFrame constructor
     *
     * @tparam Q - Template parameter to resolve to FrameMetaData
     * @param block_ptr - address of preallocated block memory for the DataBlock
     * @param block_size - size of data in bytes of the preallocated block memory
     * @param meta_data - frame's FrameMetaData
     * @param image_offset - between start of data memory and image
     */
    template <
        typename Q,
        typename = typename std::enable_if<std::is_same<typename std::decay<Q>::type, FrameMetaData>::value>::type>
    DataBlockFrame(void* block_ptr, size_t block_size, Q&& meta_data, const int image_offset = 0) :
        Frame(std::forward<Q>(meta_data), block_size, image_offset),
        is_preallocated_ { true },
        data_block_ { block_ptr, block_size }
    {
    }

    /** Delete copy constructor */
    DataBlockFrame(const DataBlockFrame& frame) = delete;

    /** Delete copy assignment */
    DataBlockFrame& operator=(DataBlockFrame& frame) = delete;

    /** move constructor */
    DataBlockFrame(DataBlockFrame&& frame) = default;

    /** Move assignment operator*/
    DataBlockFrame& operator=(DataBlockFrame&& frame)
    {
        if (this != &frame) {
            Frame::operator=(std::move(frame));
            if (!is_preallocated_ & (data_block_.get_writeable_data() != nullptr)) {
                UDataBlockPool::release(data_block_.get_writeable_data());
            }
            data_block_ = std::move(frame.data_block_);
            is_preallocated_ = frame.is_preallocated_;
        }
        return *this;
    }

    /** Destructor */
    ~DataBlockFrame() noexcept
    {
        if (!is_preallocated_ & (data_block_.get_writeable_data() != nullptr)) {
            UDataBlockPool::release(data_block_.get_writeable_data());
        }
    }

    /** Return a void pointer to the raw data */
    virtual void* get_data_ptr() const
    {
        return data_block_.get_writeable_data();
    }

private:
    /** Boolean to indicate if data_block_ memory was preallocated/internally allocated */
    bool is_preallocated_;
    /** data block */
    DataBlock data_block_;
};

}

#endif // DATABLOCKFRAME_H
