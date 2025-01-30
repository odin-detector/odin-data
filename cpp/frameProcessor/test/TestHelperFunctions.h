#ifndef FRAMEPROCESSOR_TESTHELPERFUNCTIONS_H
#define FRAMEPROCESSOR_TESTHELPERFUNCTIONS_H

#include <memory>

static std::shared_ptr<FrameProcessor::DataBlockFrame> get_dummy_frame() {

  dimensions_t dims(2, 0);
  FrameProcessor::FrameMetaData frame_meta(0, "raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression);

  char dummy_data[2] = {0, 0};

  std::shared_ptr<FrameProcessor::DataBlockFrame> frame(
							  new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), 2));

  return frame;

}

#endif
