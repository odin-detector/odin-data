#ifndef ENDOFACQUISITIONFRAME_H
#define ENDOFACQUISITIONFRAME_H

#include "Frame.h"

namespace FrameProcessor {

class EndOfAcquisitionFrame : public Frame {

public:
    /** Construct an End Of AcquisitionFrame */
    EndOfAcquisitionFrame() :
        Frame(FrameMetaData { 0, "", raw_unknown, "", std::vector<unsigned long long> {} }, 0, true)
    {
    }

    /** Return a null pointer (no raw data) */
    virtual void* get_data_ptr() const
    {
        return NULL;
    }
};

}

#endif
