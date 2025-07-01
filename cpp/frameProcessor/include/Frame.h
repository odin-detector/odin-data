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
  Frame(const FrameMetaData &meta_data, const size_t &data_size, const int &image_offset = 0);
  /** Copy constructor: Deep-copy */
  Frame(const Frame &frame);
  /** Move constructor */
  Frame(Frame&& frame);
  /** Copy Assignment operator: Deep-copy assignment */
  Frame &operator=(const Frame& frame);
  /** Move Assignment operator */
  Frame &operator=(Frame&& frame);

  /** Return if frame is valid */
  bool is_valid() const;

  /** Return a void pointer to the raw data */
  virtual void *get_data_ptr() const = 0;

  /** Return a void pointer to the image data */
  virtual void *get_image_ptr() const;

  /** Return if this frame object is an "end of acquisition" frame*/
  virtual bool get_end_of_acquisition() const;

  /** Return the data size */
  size_t get_data_size() const;

  /** Update the data size */
  void set_data_size(size_t size);

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

  /** Set MetaData */
  void set_meta_data(const FrameMetaData &meta_data);

  /** Return the image size */
  int get_image_size() const;

  /** Set the image size */
  void set_image_size(const int &size);

  /** Set the image offset */
  void set_image_offset(const int &offset);

  /** Set the outer chunk size */
  void set_outer_chunk_size(const int &size);

  /** Set the outer chunk size */
  int get_outer_chunk_size() const;

 protected:

  /** Pointer to logger */
  log4cxx::LoggerPtr logger_;

  /** Frame MetaData */
  FrameMetaData meta_data_;

  /** Shared memory size **/
  size_t data_size_;

  /** Size of the image **/
  int image_size_;

  /** Offset from frame memory start to image data */
  int image_offset_;

  /** Outer chunk size of this frame (number of images in this chunk) */
  int outer_chunk_size_;

};

}

#endif
