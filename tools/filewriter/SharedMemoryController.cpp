/*
 * SharedMemoryController.cpp
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#include <SharedMemoryController.h>

namespace filewriter
{

  /** Constructor.
   * The constructor sets up logging used within the class.
   */
  SharedMemoryController::SharedMemoryController()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("SharedMemoryController");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "SharedMemoryController constructor.");
  }

  SharedMemoryController::~SharedMemoryController()
  {
    // TODO Auto-generated destructor stub
  }

  /** setSharedMemoryParser
   * Takes a shared pointer to a shared memory parser and stores it in
   * a member variable.
   *
   * \param[in] smp - shared pointer to a SharedMemoryParser object.
   */
  void SharedMemoryController::setSharedMemoryParser(boost::shared_ptr<SharedMemoryParser> smp)
  {
    smp_ = smp;
  }

  /** setFrameReleasePublisher
   * Takes a shared pointer to a JSONPublisher and stores it in a member
   *  variable.
   *
   * \param[in] frp - shared pointer to a JSONPublisher object.
   */
  void SharedMemoryController::setFrameReleasePublisher(boost::shared_ptr<JSONPublisher> frp)
  {
    frp_ = frp;
  }

  void SharedMemoryController::callback(boost::shared_ptr<JSONMessage> msg)
  {
    LOG4CXX_DEBUG(logger_, "Parsed json: " << msg->toString());

    // Copy the data out into a Frame object
    unsigned int buffer_id = (*msg)["params"]["buffer_id"].GetInt64();
    LOG4CXX_DEBUG(logger_, "Creating Reset Frame object. buffer=" << buffer_id
                        << " buffer addr: " << smp_->get_buffer_address(buffer_id));
    LOG4CXX_DEBUG(logger_, "  Header addr: " << smp_->get_frame_header_address(buffer_id)
                        << "  Data addr: " << smp_->get_reset_data_address(buffer_id));



    // Assuming this is a P2M, our image dimensions are:
    size_t bytes_per_pixel = 2;
    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;
    dimensions_t p2m_subframe_dims = p2m_dims; p2m_subframe_dims[1] = p2m_subframe_dims[1] >> 1;

    boost::shared_ptr<Frame> reset_frame;
    reset_frame = boost::shared_ptr<Frame>(new Frame(bytes_per_pixel, p2m_dims));
    reset_frame->set_dataset_name("reset");
    reset_frame->set_subframe_dimensions(p2m_subframe_dims);
    smp_->get_reset_frame(*reset_frame, buffer_id);

    LOG4CXX_DEBUG(logger_, "Creating Data Frame object. buffer=" << buffer_id);
    LOG4CXX_DEBUG(logger_,"  Data addr: " << smp_->get_frame_data_address(buffer_id));
    boost::shared_ptr<Frame> frame;
    frame = boost::shared_ptr<Frame>(new Frame(bytes_per_pixel, p2m_dims));
    frame->set_dataset_name("data");
    frame->set_subframe_dimensions(p2m_subframe_dims);
    smp_->get_frame(*frame, buffer_id);

    // Loop over callbacks, plaing both frames onto each queue
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
    for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
      cbIter->second->getWorkQueue()->add(frame);
      cbIter->second->getWorkQueue()->add(reset_frame);
    }


    // We want to return the exact same JSON message back to the frameReceiver
    // so we just modify the relevant bits: msg_val: from frame_ready to frame_release
    // and update the timestamp
    (*msg)["msg_val"].SetString("frame_release");

    // Create and update the timestamp in the JSON message
    boost::posix_time::ptime msg_timestamp = boost::posix_time::microsec_clock::local_time();
    std::string msg_timestamp_str = boost::posix_time::to_iso_extended_string(msg_timestamp);
    (*msg)["timestamp"].SetString(StringRef(msg_timestamp_str.c_str()));

    // Now publish the release message
    frp_->publish(msg);
  }

  void SharedMemoryController::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb)
  {
    // Record the callback pointer
    callbacks_[name] = cb;
    // Start the callback thread
    cb->start();
  }


} /* namespace filewriter */
