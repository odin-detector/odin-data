#ifndef FRAMEPROCESSOR_FRAME_H
#define FRAMEPROCESSOR_FRAME_H

#include <string>
#include <stdint.h>

#include <log4cxx/logger.h>
#include "FrameMetaData.h"

namespace FrameProcessor {

/** Interface class for a Frame; all Frames must sub-class this.
 *
 */
class Frame {

 public:

  /** Base constructor */
  Frame(const FrameMetaData &meta_data, const int &image_offset = 0);
  /** Shallow-copy copy */
  Frame(const Frame &frame);

  /** Deep-copy assignment */
  Frame &operator=(const Frame& frame);

  /** Return if frame is valid */
  bool is_valid() const;

  /** Return a void pointer to the raw data */
  virtual void *get_data_ptr() const = 0;

  /** Return a void pointer to the image data */
  virtual void *get_image_ptr() const;

  /** Return the data size */
  virtual size_t get_data_size() const = 0;

  /** Change the data size */
  virtual void resize(size_t size) = 0;

  /** Return the frame number */
  long long get_frame_number() const;

  /** Set the frame number */
  void set_frame_number(long long frame_number);

  /** Return a reference to the MetaData */
  FrameMetaData &meta_data();

  /** Return the MetaData */
  const FrameMetaData &get_meta_data() const;

  /** Return deep copy of the MetaData */
  FrameMetaData get_meta_data_copy() const;

  /** Return the image offset */
  int get_image_offset() const;

  /** Set MetaData */
  void set_meta_data(const FrameMetaData &meta_data);

  /** Set the image offset */
  void set_image_offset(const int &offset);

 protected:

  /** Pointer to logger */
  log4cxx::LoggerPtr logger;

  /** Frame MetaData */
  FrameMetaData meta_data_;

  /** Offset from frame memory start to image data */
  int image_offset_;

};

}

#endif
