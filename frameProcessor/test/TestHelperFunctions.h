#ifndef FRAMEPROCESSOR_TESTHELPERFUNCTIONS_H
#define FRAMEPROCESSOR_TESTHELPERFUNCTIONS_H

#include <boost/shared_ptr.hpp>

static boost::shared_ptr<FrameProcessor::DataBlockFrame> get_dummy_frame() {

  dimensions_t dims(2, 0);
  FrameProcessor::IFrameMetaData frame_meta("raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression);

  char dummy_data[2] = {0, 0};

  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
							  new FrameProcessor::DataBlockFrame(0, frame_meta, static_cast<void*>(dummy_data), 2));

  return frame;

}

#endif
