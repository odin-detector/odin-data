#include "EndOfAcquisitionFrame.h"

namespace FrameProcessor {

/** EndOfAcquisitionFrame constructor
 */
EndOfAcquisitionFrame::EndOfAcquisitionFrame() :
    Frame(meta_data_, 0, 0)
{
}

/** Copy constructor;
 * implement as shallow copy
 * @param frame
 */
EndOfAcquisitionFrame::EndOfAcquisitionFrame(const EndOfAcquisitionFrame &frame) : Frame(frame) {
};

/** Assignment operator;
 * implement as deep copy
 * @param frame
 * @return Frame
 */
EndOfAcquisitionFrame &EndOfAcquisitionFrame::operator=(EndOfAcquisitionFrame &frame) {
  Frame::operator=(frame);
  return *this;
}

/** Destroy frame
 *
 */
EndOfAcquisitionFrame::~EndOfAcquisitionFrame() {
}

/** No data ptr so return null
 *
 */
void *EndOfAcquisitionFrame::get_data_ptr() const {
  return NULL;
}

/** Return true to signify that this frame
 * is an "end of acquisition" frame.
 * @ return end of acquisition
 */
bool EndOfAcquisitionFrame::get_end_of_acquisition() const {
  return true;
}

}
