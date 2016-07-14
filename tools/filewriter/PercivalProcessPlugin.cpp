/*
 * PercivalProcessPlugin.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#include <PercivalProcessPlugin.h>

namespace filewriter
{

  PercivalProcessPlugin::PercivalProcessPlugin()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FW.PercivalProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "PercivalProcessPlugin constructor.");
  }

  PercivalProcessPlugin::~PercivalProcessPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  void PercivalProcessPlugin::processFrame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Processing raw frame.");
    // This plugin will take the raw buffer and convert to two percival frames
    // One reset frame and one data frame
    // Assuming this is a P2M, our image dimensions are:
    size_t bytes_per_pixel = 2;
    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;
    dimensions_t p2m_subframe_dims = p2m_dims; p2m_subframe_dims[1] = p2m_subframe_dims[1] >> 1;
    size_t subframe_bytes = p2m_subframe_dims[0] * p2m_subframe_dims[1] * bytes_per_pixel;

    // Read out the frame header from the raw frame
    const PercivalEmulator::FrameHeader* hdrPtr = static_cast<const PercivalEmulator::FrameHeader*>(frame->get_data());
    LOG4CXX_TRACE(logger_, "Raw frame number: " << hdrPtr->frame_number);

    boost::shared_ptr<Frame> reset_frame;
    reset_frame = boost::shared_ptr<Frame>(new Frame("reset"));
    reset_frame->set_frame_number(hdrPtr->frame_number);
    reset_frame->set_dimensions("frame", p2m_dims);
    reset_frame->set_dimensions("subframe", p2m_subframe_dims);
    reset_frame->set_parameter("subframe_count", PercivalEmulator::num_subframes);
    reset_frame->set_parameter("subframe_size", PercivalEmulator::subframe_size);
    // Copy data into frame
    reset_frame->copy_data((static_cast<const char*>(frame->get_data())+sizeof(PercivalEmulator::FrameHeader)+PercivalEmulator::data_type_size), PercivalEmulator::data_type_size);
    LOG4CXX_TRACE(logger_, "Pushing reset frame.");
    this->push(reset_frame);

    //LOG4CXX_DEBUG(logger_, "Creating Data Frame object. buffer=" << buffer_id);
    //LOG4CXX_DEBUG(logger_,"  Data addr: " << smp_->get_frame_data_address(buffer_id));
    boost::shared_ptr<Frame> data_frame;
    data_frame = boost::shared_ptr<Frame>(new Frame("data"));
    data_frame->set_frame_number(hdrPtr->frame_number);
    data_frame->set_dimensions("frame", p2m_dims);
    data_frame->set_dimensions("subframe", p2m_subframe_dims);
    data_frame->set_parameter("subframe_count", PercivalEmulator::num_subframes);
    data_frame->set_parameter("subframe_size", PercivalEmulator::subframe_size);
    data_frame->copy_data((static_cast<const char*>(frame->get_data())+sizeof(PercivalEmulator::FrameHeader)), PercivalEmulator::data_type_size);
    LOG4CXX_TRACE(logger_, "Pushing data frame.");
    this->push(data_frame);

  }

} /* namespace filewriter */
