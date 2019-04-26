#include "SharedBufferFrame.h"

#include "IpcMessage.h"

namespace FrameProcessor {

SharedBufferFrame::SharedBufferFrame(const FrameMetaData &meta_data,
                                     void* data_src,
                                     size_t nbytes,
                                     uint64_t bufferID,
                                     OdinData::IpcChannel *relCh,
                                     const int& image_offset) : Frame(meta_data, nbytes, image_offset) {
  data_ptr_ = data_src;
  shared_id_ = bufferID;
  shared_channel_ = relCh;
}

/** Copy constructor;
 * implement as shallow copy
 * @param frame
 */
SharedBufferFrame::SharedBufferFrame(const SharedBufferFrame& frame) : Frame(frame) {
  data_ptr_ = frame.data_ptr_;
  data_size_ = frame.data_size_;
  shared_id_ = frame.shared_id_;
}

/** Destroy frame
 *
 */
SharedBufferFrame::~SharedBufferFrame () {
  OdinData::IpcMessage txMsg(OdinData::IpcMessage::MsgTypeNotify,
                             OdinData::IpcMessage::MsgValNotifyFrameRelease);
  txMsg.set_param("frame", meta_data_.get_dataset_name());
  txMsg.set_param("buffer_id", shared_id_);
  // Now publish the release message, to notify the frame receiver that we are
  // finished with that block of shared memory
  shared_channel_->send(txMsg.encode());
}

/** Return a void pointer to the raw data.
 *
 * \return pointer to the raw data.
 */
void *SharedBufferFrame::get_data_ptr() const {
  return data_ptr_;
}

}
