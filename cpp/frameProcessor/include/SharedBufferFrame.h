#ifndef FRAMEPROCESSOR_SHAREDBUFFERFRAME_H
#define FRAMEPROCESSOR_SHAREDBUFFERFRAME_H

#include "Frame.h"
#include "IpcChannel.h"
#include "IpcMessage.h"
#include <functional>
#include <stdint.h>

namespace FrameProcessor {

class SharedBufferFrame : public Frame {

public:
    /** Construct a SharedBufferFrame */
    template <
        typename Q,
        typename = typename std::enable_if<std::is_same<typename std::decay<Q>::type, FrameMetaData>::value>::type>
    SharedBufferFrame(
        Q&& meta_data,
        void* data_src,
        size_t nbytes,
        uint64_t bufferID,
        OdinData::IpcChannel& relCh
    ) noexcept(std::is_rvalue_reference<Q>::value) :
        Frame { std::forward<Q>(meta_data), nbytes },
        data_ptr_ { data_src },
        shared_id_ { bufferID },
        shared_channel_ { relCh }
    {
    }

    /** Delete copy constructor - no copy semantics!
     */
    SharedBufferFrame(const SharedBufferFrame& frame) = delete;

    /** Delete copy assignment operator
     * In the instance this object has a valid data_ptr_ We don't want to
     * leak memory or resources.
     */
    SharedBufferFrame& operator=(const SharedBufferFrame& frame) = delete;

    /** move constructor */
    SharedBufferFrame(SharedBufferFrame&& frame) noexcept :
        Frame(std::move(frame)),
        data_ptr_ { frame.data_ptr_ },
        shared_id_ { frame.shared_id_ },
        shared_channel_ { frame.shared_channel_ }
    {
    }

    /** Delete move assignment operator
     * In the instance this object has a valid data_ptr_ We don't want to
     * leak memory or resources.
     */
    SharedBufferFrame& operator=(SharedBufferFrame&& frame) = delete;

    /** Destructor */
    ~SharedBufferFrame()
    {
        if (data_ptr_ == nullptr)
            return;
        OdinData::IpcMessage txMsg(OdinData::IpcMessage::MsgTypeNotify, OdinData::IpcMessage::MsgValNotifyFrameRelease);
        txMsg.set_param("frame", static_cast<uint64_t>(meta_data_.get_frame_number()));
        txMsg.set_param("buffer_id", shared_id_);
        // Now publish the release message, to notify the frame receiver that we are
        // finished with that block of shared memory
        shared_channel_.get().send(txMsg.encode());
    }

    /** Return a void pointer to the raw data.
     *
     * @return pointer to the raw data.
     */
    virtual void* get_data_ptr() const
    {
        return data_ptr_;
    }

private:
    /** Pointer to shared memory raw block **/
    void* data_ptr_;

    /** Shared memory buffer ID **/
    uint64_t shared_id_;

    /** ZMQ release channel for the shared buffer **/
    std::reference_wrapper<OdinData::IpcChannel> shared_channel_;
};

}

#endif
