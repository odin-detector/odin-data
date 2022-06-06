#include "WrapperFrame.h"

namespace FrameProcessor {

WrapperFrame::WrapperFrame(
  boost::shared_ptr<Frame> frame, const int& image_offset
) :
  Frame(frame->get_meta_data_copy(), frame->get_data_size(), image_offset)
{
  this->wrapped_frame_ = frame;
}

// Re-implement getters as a view onto the real Frame

void* WrapperFrame::get_data_ptr() const {
  return this->wrapped_frame_->get_data_ptr();
}

size_t WrapperFrame::get_data_size() const {
  return this->wrapped_frame_->get_data_size();
}

long long WrapperFrame::get_frame_number() const {
  return this->wrapped_frame_->get_frame_number();
}

// These methods are disabled as this is a read-only view of a Frame
void WrapperFrame::set_data_size(size_t size) {}
void WrapperFrame::set_frame_number() const {}

}
