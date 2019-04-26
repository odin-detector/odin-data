#include "Frame.h"
#include "FrameMetaData.h"

namespace FrameProcessor {

/** Base Frame constructor
 *
 * @param meta-data - frame FrameMetaData
 * @param image_offset - between start of data memory and image
 */
    Frame::Frame(const FrameMetaData& meta_data, const int &image_offset) :
            meta_data_(meta_data),
            image_offset_(image_offset),
            logger(log4cxx::Logger::getLogger("FP.Frame")) {
      logger->setLevel(log4cxx::Level::getAll());
    }

/** Copy constructor;
 * implement as shallow copy
 * @param frame - source frame
 */
    Frame::Frame(const Frame &frame) {
      meta_data_ = frame.meta_data_;
      image_offset_ = frame.image_offset_;
      logger = frame.logger;
    }

/** Assignment operator;
 * implement as deep copy
 * @param frame - source frame
 * @return Frame
 */
    Frame &Frame::operator=(const Frame &frame) {
      meta_data_ = frame.meta_data_;
      image_offset_ = frame.image_offset_;
      logger = frame.logger;
      return *this;
    }

/** Return if frame is valid
 * @return bool - frame is valid
 */
    bool Frame::is_valid() const {
      if (meta_data_.get_data_type() == raw_unknown)
        return false;
      if (meta_data_.get_compression_type() == unknown_compression)
        return false;
      return true;
    }
    
    /** Return a void pointer to the image data
 * @ return void poiter to image data
 */
    void *Frame::get_image_ptr() const {
      return this->get_data_ptr() + image_offset_;
    }

/** Return the frame number
 * @return frame frame number
 */
    long long Frame::get_frame_number() const {
      return this->meta_data_.get_frame_number();
    }

/** Set the frame number
 * @param frame_number - new frame number
 */
    void Frame::set_frame_number(long long frame_number) {
      this->meta_data_.set_frame_number(frame_number);
    }

/** Return a reference to the MetaData
 * @return reference to meta data
 */
    FrameMetaData &Frame::meta_data() {
      return this->meta_data_;
    }

/** Return the MetaData
 * @return frame meta data
 */
    const FrameMetaData &Frame::get_meta_data() const {
      return this->meta_data_;
    }

/** Return deep copy of the MetaData
 * @return FrameMetaDatacopy (deep) of the meta data
 */
    FrameMetaData Frame::get_meta_data_copy() const {
      return this->meta_data_;
    }

/** Return the image offset
 * @return offset from beginning of data memory to image data
 * */
    int Frame::get_image_offset() const {
      return this->image_offset_;
    }

/** Set MetaData
 * @param FrameMetaData meta_data - new meta data
 * */
    void Frame::set_meta_data(const FrameMetaData &meta_data) {
      this->meta_data_ = meta_data;
    }

/** Set the image offset
 * @param offset - new offset
 * */
    void Frame::set_image_offset(const int &offset) {
      this->image_offset_ = offset;
    }

}
