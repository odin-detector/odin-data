#ifndef FRAMEPROCESSOR_FRAME_H
#define FRAMEPROCESSOR_FRAME_H

#include <stdint.h>
#include <string>

#include "FrameMetaData.h"
#include <log4cxx/logger.h>

namespace FrameProcessor {

/** Interface class for a Frame; all Frames must sub-class this.
 *
 */
class Frame {

public:
    /** Base constructor */
    Frame(const FrameMetaData& meta_data, const uint64_t data_size, bool is_acq_end = false) :
        meta_data_(meta_data),
        data_size_(data_size),
        image_size_(data_size - meta_data_.frame_offset_),
        outer_chunk_size_(1),
        is_end_of_acq_ { is_acq_end }
    {
    }

    /** Base constructor */
    Frame(FrameMetaData&& meta_data, const uint64_t data_size, bool is_acq_end = false) :
        meta_data_(std::move(meta_data)),
        data_size_(data_size),
        image_size_(data_size - meta_data_.frame_offset_),
        outer_chunk_size_(1),
        is_end_of_acq_ { is_acq_end }
    {
    }

    /** Copy constructor
     * @param frame - source frame
     */
    Frame(const Frame& frame) = default;

    /** Move constructor
     * @param frame - source frame
     */
    Frame(Frame&& frame) = default;

    /** Copy assignment operator
     * @param frame - source frame
     * @return this object
     */
    Frame& operator=(const Frame& frame) = default;

    /** Move Assignment operator
     * @param frame - source frame
     * @return this object
     */
    Frame& operator=(Frame&& frame) = default;

    /** Return if frame is valid
     * @return bool - frame is valid
     */
    bool is_valid() const
    {
        bool b_array[2] = { true, false };
        return b_array[(meta_data_.data_type_ == raw_unknown) | (meta_data_.compression_type_ == unknown_compression)];
    }

    /** Return a void pointer to the raw data */
    virtual void* get_data_ptr() const = 0;

    /** Return a void pointer to the image data */
    virtual void* get_image_ptr() const
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(this->get_data_ptr()) + meta_data_.frame_offset_);
    }

    /** Return the data size
     *  @return bool - default behaviour is false, can be overriden
     *                 by settting it to true in a child class
     */
    bool get_end_of_acquisition() const
    {
        return is_end_of_acq_;
    }

    /** Return the data size
     *  @return data_size_
     */
    size_t get_data_size() const
    {
        return data_size_;
    }

    /** Update the data size
     * @param size new data size
     */
    void set_data_size(size_t size)
    {
        if (size < this->data_size_) {
            LOG4CXX_DEBUG(log4cxx::Logger::getLogger("FP.Frame"), "Updating size of frame data");
            data_size_ = size;
        }
        LOG4CXX_DEBUG(
            log4cxx::Logger::getLogger("FP.Frame"), "Can't set frame data size to be bigger than allocated memory size."
        );
    }

    /** Return the frame number
     *  @return frame frame number
     */
    long long get_frame_number() const
    {
        return this->meta_data_.frame_number_;
    }

    /** Set the frame number */
    void set_frame_number(long long frame_number)
    {
        this->meta_data_.frame_number_ = frame_number;
    }

    /** @return a reference to the MetaData */
    FrameMetaData& meta_data()
    {
        return this->meta_data_;
    }

    /** @return a MetaData const reference */
    const FrameMetaData& get_meta_data() const
    {
        return meta_data_;
    }

    /** @return a copy of the MetaData */
    FrameMetaData get_meta_data_copy() const
    {
        return meta_data_;
    }

    /** Set acquisition ID
     * @tparam T - FrameMetaData type
     * @param metadata - metadata object to set
     */
    template <
        typename T,
        typename = typename std::enable_if<std::is_same<typename std::decay<T>::type, FrameMetaData>::value>::type>
    void set_meta_data(T&& metadata) noexcept(
        std::is_same<typename std::decay<T>::type, FrameMetaData>::value && std::is_rvalue_reference<T>::value
    )
    {
        meta_data_ = std::move(metadata);
    }

    /** Return the image size
     * @return size of data minus offset (header size)
     */
    uint64_t get_image_size() const
    {
        return image_size_;
    }

    /** Set the image size
     * @param size the image size
     */
    void set_image_size(const size_t size)
    {
        image_size_ = size;
    }

    /** Set the image offset
     * @param offset - new offset
     */
    void set_image_offset(const int64_t offset)
    {
        meta_data_.frame_offset_ = offset; // update the metadata's offset value
        this->image_size_ = this->data_size_ - offset; // set the data size
    }

    /** Set the outer chunk size of the frame (number of trigger acqs in chunk)
     * @param size - outer_chunk_size
     */
    void set_outer_chunk_size(const int size)
    {
        outer_chunk_size_ = size;
    }

    /** Get the outer chunk size of the frame (number of trigger acqs in chunk)
     * @return outer_chunk_size
     */
    int get_outer_chunk_size() const
    {
        return outer_chunk_size_;
    }

protected:
    /** Frame MetaData */
    FrameMetaData meta_data_;

    /** Shared memory size **/
    uint64_t data_size_;

    /** Size of the image **/
    uint64_t image_size_;

    /** Outer chunk size of this frame (number of images in this chunk) */
    int outer_chunk_size_;

    /** Boolean indicating if the frame is an end of acquisiition or not */
    bool is_end_of_acq_;
};

}

#endif
