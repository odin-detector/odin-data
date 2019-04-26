#include "DataBlockFrame.h"
#include "DataBlockPool.h"

namespace FrameProcessor {

/** DataBlockFrame constructor
 *
 * @param meta_data - frame FrameMetaData
 * @param data_src - data to copy into block memory
 * @param block_size - size of data in bytes
 * @param image_offset - between start of data memory and image
 */
DataBlockFrame::DataBlockFrame(const FrameMetaData &meta_data,
                               const void* data_src,
                               size_t block_size,
                               const int& image_offset) :
    Frame(meta_data, image_offset)
{
  raw_data_block_ptr_ = DataBlockPool::take(block_size);
  raw_data_block_ptr_->copy_data(data_src, block_size);
}

/** DataBlockFrame constructor
 *
 * @param meta_data - frame FrameMetaData
 * @param block_size - size of data in bytes
 * @param image_offset - between start of data memory and image
 */
DataBlockFrame::DataBlockFrame(const FrameMetaData &meta_data,
                               size_t block_size,
                               const int &image_offset) : Frame(meta_data, image_offset)
{
  raw_data_block_ptr_ = DataBlockPool::take(block_size);
}

/** Copy constructor;
 * implement as shallow copy
 * @param frame
 */
DataBlockFrame::DataBlockFrame(const DataBlockFrame &frame) : Frame(frame) {
  raw_data_block_ptr_ = frame.raw_data_block_ptr_;
};

/** Assignment operator;
 * implement as deep copy
 * @param frame
 * @return Frame
 */
DataBlockFrame &DataBlockFrame::operator=(DataBlockFrame &frame) {
  Frame::operator=(frame);
  if (raw_data_block_ptr_) {
    DataBlockPool::release(raw_data_block_ptr_);
    raw_data_block_ptr_ = DataBlockPool::take(frame.get_data_size());
  }
  raw_data_block_ptr_->copy_data(frame.get_data_ptr(), frame.get_data_size());
  return *this;
}

/** Destroy frame
 *
 */
DataBlockFrame::~DataBlockFrame() {
  DataBlockPool::release(raw_data_block_ptr_);
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
  return raw_data_block_ptr_->get_size();
}

/** Set the data size */
void DataBlockFrame::resize(size_t size) {
  // TO DO
}


}
