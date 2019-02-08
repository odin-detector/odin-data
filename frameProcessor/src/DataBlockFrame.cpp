#include "DataBlockFrame.h"
#include "DataBlockPool.h"

namespace FrameProcessor {

    /** DataBlockFrame constructor
     *
     * @param meta_data - frame IFrameMetaData
     * @param data_src - data to copy into block memory
     * @param nbytes - size of data
     * @param image_offset - between start of data memory and image
     */
    DataBlockFrame::DataBlockFrame(const IFrameMetaData &meta_data,
                                 const void *data_src,
                                 size_t nbytes,
                                 const int &image_offset) : IFrame(meta_data, image_offset) {

        //raw_data_block_ptr_ = DataBlockPool::take(nbytes, nbytes); TO DO: modify DataBlockPool to take nbytes as 'identifier'
        raw_data_block_ptr_->copyData(data_src, nbytes);

    }

    /** DataBlockFrame constructor
     *
     * @param meta_data - frame IFrameMetaData
     * @param nbytes - size of data
     * @param image_offset - between start of data memory and image
     */
     DataBlockFrame::DataBlockFrame(const IFrameMetaData &meta_data,
                                   size_t nbytes,
                                   const int &image_offset) : IFrame(meta_data, image_offset) {

        //raw_data_block_ptr_ = DataBlockPool::take(nbytes, nbytes); TO DO: modify DataBlockPool to take nbytes as 'identifier'

    }

    /** Copy constructor;
     * implement as shallow copy
     * @param frame
     */
    DataBlockFrame::DataBlockFrame(const DataBlockFrame &frame) : IFrame(frame) {
        raw_data_block_ptr_ = frame.raw_data_block_ptr_;
    };

    /** Assignment operator;
     * implement as deep copy
     * @param frame
     * @return IFrame
     */
    DataBlockFrame &DataBlockFrame::operator=(DataBlockFrame &frame) {
        IFrame::operator=(frame);
        if (raw_data_block_ptr_) {
          //DataBlockPool::release(this->get_data_size(), raw_data_block_ptr_); TO DO: modify DataBlockPool to take nbytes as 'identifier'
        }
        //raw_data_block_ptr_ =  DataBlockPool::take(frame.get_data_size(), frame.get_data_size()); TO DO: modify DataBlockPool to take nbytes as 'identifier'
        raw_data_block_ptr_->copyData(frame.get_data_ptr(), frame.get_data_size());
        return *this;
    }

    /** Destroy frame
     *
     */
    DataBlockFrame::~DataBlockFrame() {
        //DataBlockPool::release(this->get_data_size(), raw_data_block_ptr_); TO DO: modify DataBlockPool to take nbytes as 'identifier'
    }

    /** Return a void pointer to the raw data.
     *
     * \return pointer to the raw data.
     */
    void *DataBlockFrame::get_data_ptr() const {
        return raw_data_block_ptr_->get_writeable_data();
    }

    /** Get data size
     *
     * @return size_t data size
     */
    size_t DataBlockFrame::get_data_size() const {
        return raw_data_block_ptr_->getSize();
    }

}
