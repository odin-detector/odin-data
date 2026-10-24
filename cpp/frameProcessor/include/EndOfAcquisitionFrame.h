#ifndef FRAMEPROCSESOR_ENDOFACQUISITIONFRAME_H
#define FRAMEPROCESSOR_ENDOFACQUISITIONFRAME_H

#include "Frame.h"

namespace FrameProcessor {

class EndOfAcquisitionFrame : public Frame {

public:
    /** Construct an End Of AcquisitionFrame */
    EndOfAcquisitionFrame();

    /** Shallow-copy copy */
    EndOfAcquisitionFrame(const EndOfAcquisitionFrame& frame);

    /** Deep-copy assignment */
    EndOfAcquisitionFrame& operator=(EndOfAcquisitionFrame& frame);

    /** Destructor */
    ~EndOfAcquisitionFrame();

    /** Return a null pointer (no raw data) */
    virtual void* get_data_ptr() const;
};

}

#endif
