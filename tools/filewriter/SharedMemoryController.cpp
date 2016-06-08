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
    LOG4CXX_TRACE(logger_, "SharedMemoryController destructor.");
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
    LOG4CXX_DEBUG(logger_, "Callback with message: " << msg->toString());

    // Get the shared memory buffer ID from the message
    unsigned int buffer_id = (*msg)["params"]["buffer_id"].GetInt64();
    LOG4CXX_DEBUG(logger_, "Creating Raw Frame object. buffer=" << buffer_id <<
                           " buffer addr: " << smp_->get_buffer_address(buffer_id));

    // Create a frame object and copy in the raw frame data
    boost::shared_ptr<Frame> frame;
    frame = boost::shared_ptr<Frame>(new Frame("raw"));
    smp_->get_frame((*frame), buffer_id);

    // Loop over registered callbacks, placing the frame onto each queue
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
    for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
      cbIter->second->getWorkQueue()->add(frame);
    }

    // We want to return the exact same JSON message back to the frameReceiver
    // so we just modify the relevant bits: msg_val: from frame_ready to frame_release
    // and update the timestamp
    (*msg)["msg_val"].SetString("frame_release");

    // Create and update the timestamp in the JSON message
    boost::posix_time::ptime msg_timestamp = boost::posix_time::microsec_clock::local_time();
    std::string msg_timestamp_str = boost::posix_time::to_iso_extended_string(msg_timestamp);
    (*msg)["timestamp"].SetString(StringRef(msg_timestamp_str.c_str()));

    // Now publish the release message, to notify the frame receiver that we are
    // finished with that block of shared memory
    frp_->publish(msg);
  }

  void SharedMemoryController::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb)
  {
    // Check if we own the callback already
    if (callbacks_.count(name) == 0){
      // Record the callback pointer
      callbacks_[name] = cb;
      // Confirm registration
      cb->confirmRegistration("frame_receiver");
    }
  }

  void SharedMemoryController::removeCallback(const std::string& name)
  {
    boost::shared_ptr<IFrameCallback> cb;
    if (callbacks_.count(name) > 0){
      // Get the pointer
      cb = callbacks_[name];
      // Remove the callback from the map
      callbacks_.erase(name);
      // Confirm removal
      cb->confirmRemoval("frame_receiver");
    }
  }

} /* namespace filewriter */
