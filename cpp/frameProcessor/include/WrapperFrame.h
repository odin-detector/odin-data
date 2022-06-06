#ifndef FRAMEPROCESSOR_WRAPPER_FRAME_H
#define FRAMEPROCESSOR_WRAPPER_FRAME_H

#include <string>
#include <stdint.h>

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>

#include "Frame.h"
#include "FrameMetaData.h"

namespace FrameProcessor {

/** A read-only view of the underlying memory of a `Frame`
 *
 * When `Frame` data contains multiple images, a `WrapperFrame` can be used to publish a
 * distinct `Frame` for each image without copying the data by setting the offset to each
 * image in the underlying `Frame` data.
 *
 * Some attributes are re-implmented as a view onto the `Frame` - e.g. the
 * data pointer and frame number - while others are distinct - e.g. the data offset and
 * meta data. The meta data is initially copied from the `Frame` and can then be updated.
 *
 * \param[in] frame - The underlying `Frame`
 * \param[in] image_offset - Offset from underlying `Frame` data pointer to image data
 *
 */
class WrapperFrame : public Frame {
public:
  WrapperFrame(boost::shared_ptr<Frame> frame, const int& image_offset);

  // Base class virtual methods
  void* get_data_ptr() const;
  size_t get_data_size() const ;
  long long get_frame_number() const;

private:
  /** Underlying `Frame` */
  boost::shared_ptr<Frame> wrapped_frame_;

  // Override these private - this is just a read-only view of another `Frame`

  void set_data_size(size_t size);
  void set_frame_number() const;
};

}

#endif
