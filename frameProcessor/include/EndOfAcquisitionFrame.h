#ifndef FRAMEPROCSESOR_ENDOFACQUISITIONFRAME_H
#define FRAMEPROCESSOR_ENDOFACQUISITIONFRAME_H

#include "Frame.h"

namespace FrameProcessor {

class EndOfAcquisitionFrame : public Frame {

 public:

  /** Construct an End Of AcquisitionFrame */
  EndOfAcquisitionFrame();

  /** Shallow-copy copy */
  EndOfAcquisitionFrame(const EndOfAcquisitionFrame &frame);

  /** Deep-copy assignment */
  EndOfAcquisitionFrame& operator=(EndOfAcquisitionFrame &frame);

  /** Destructor */
  ~EndOfAcquisitionFrame();

  /** Return a null pointer (no raw data) */
  virtual void *get_data_ptr() const;

  /** Return confirmation that this is an "end of acquisition" frame*/
  virtual bool get_end_of_acquisition() const;
};

}

#endif
