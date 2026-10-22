/*
 * WrapperFrame.h
 *
 *  Created on: 19 Oct 2026
 *      Author: Famous Alele
 */
#ifndef WRAPPED_FRAME_H
#define WRAPPED_FRAME_H

#include <stdint.h>
#include <string>

#include <boost/shared_ptr.hpp>

#include "Frame.h"

namespace FrameProcessor {

/** A read-only view of the underlying memory of a `Frame`
 *
 * When `Frame` data contains multiple images, a `WrapperFrame` can be used to publish a
 * distinct `Frame` for each image without copying the data by setting the offset to each
 * image in the underlying `Frame` data.
 *
 * Some attributes are deleted to ensure this class is read-only.
 * The meta data is initially copied from the `Frame` and can then be updated.
 *
 * \param[in] frame - Shared pointer to the underlying original `Frame`
 * \param[in] meta_data - Metadata for this wrapper frame
 * \param[in] image_offset - Offset from underlying `Frame` data pointer to image data
 */
class WrappedFrame : public Frame {
public:
    WrappedFrame(boost::shared_ptr<Frame> frame, const FrameMetaData& metadata, const int image_offset) :
        Frame(metadata, frame->get_data_size(), image_offset),
        original_frame_(std::move(frame))
    {
    }

    WrappedFrame(boost::shared_ptr<Frame> frame, FrameMetaData&& metadata, const int image_offset) :
        Frame(std::move(metadata), frame->get_data_size(), image_offset),
        original_frame_(std::move(frame))
    {
    }

    // Base class virtual methods
    void* get_data_ptr() const
    {
        return original_frame_->get_data_ptr();
    }

    // delete base class setter methods
    void set_data_size(size_t) = delete;
    void set_meta_data(const FrameMetaData&) = delete;
    void set_image_size(const int&) = delete;
    void set_image_offset(const int&) = delete;
    void set_outer_chunk_size(const int&) = delete;

private:
    /** Underlying Frame's memory address */
    const boost::shared_ptr<Frame> original_frame_;
};

}

#endif
