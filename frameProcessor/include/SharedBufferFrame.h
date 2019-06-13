#ifndef FRAMEPROCESSOR_SHAREDBUFFERFRAME_H
#define FRAMEPROCESSOR_SHAREDBUFFERFRAME_H

#include "Frame.h"

#include <stdint.h>
#include "IpcChannel.h"

namespace FrameProcessor {

class SharedBufferFrame : public Frame {

 public:

  /** Construct a SharedBufferFrame */
  SharedBufferFrame(const FrameMetaData &meta_data,
                    void *data_src,
                    size_t nbytes,
                    uint64_t bufferID,
                    OdinData::IpcChannel *relCh,
                    const int &image_offset = 0);

  /** Shallow-copy copy */
  SharedBufferFrame(const SharedBufferFrame &frame);

  /** Destructor */
  ~SharedBufferFrame();

  /** Return a void pointer to the raw data */
  virtual void *get_data_ptr() const;

private:

  /** Pointer to shared memory raw block **/
  void *data_ptr_;

  /** Shared memory buffer ID **/
  uint64_t shared_id_;

  /** ZMQ release channel for the shared buffer **/
  OdinData::IpcChannel *shared_channel_;

};

}

#endif
