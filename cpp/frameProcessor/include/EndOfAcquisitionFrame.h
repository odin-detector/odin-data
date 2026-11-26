#ifndef ENDOFACQUISITIONFRAME_H
#define ENDOFACQUISITIONFRAME_H

#include "Frame.h"

namespace FrameProcessor {

class EndOfAcquisitionFrame : public Frame {

public:
    /** Construct an End Of AcquisitionFrame
     * Default and ONLY construcor for the EndofAcqusitionFrame class
     * The default arguments for it's parent Frame class are:
     *      @param[in] FrameMetadata invalid metadata object
     *      @param[in] data_size 0 as it has no data
     *      @param[in] is_end_acq TRUE as this is an "End of Acquisition" frame class.
     */
    EndOfAcquisitionFrame() :
        Frame(
            FrameMetaData { -1, "", raw_unknown, "", std::vector<unsigned long long> {},
                            CompressionType::unknown_compression },
            0,
            true
        )
    {
    }

    /** Return a null pointer (no raw data) */
    virtual void* get_data_ptr() const
    {
        return nullptr;
    }

    void* get_image_ptr() const = delete;
};

}

#endif
