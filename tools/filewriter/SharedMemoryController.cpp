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
  SharedMemoryController::SharedMemoryController(boost::shared_ptr<FrameReceiver::IpcReactor> reactor, const std::string& rxEndPoint, const std::string& txEndPoint) :
    reactor_(reactor),
    rxChannel_(ZMQ_SUB),
    txChannel_(ZMQ_PUB)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("SharedMemoryController");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "SharedMemoryController constructor.");

    // Connect the frame ready channel
    try {
      LOG4CXX_DEBUG(logger_, "Connecting RX Channel to endpoint: " << rxEndPoint);
      rxChannel_.connect(rxEndPoint.c_str());
      rxChannel_.subscribe("");
    }
    catch (zmq::error_t& e) {
      //std::stringstream ss;
      //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
      // TODO: What to do here, I think throw it up
      throw std::runtime_error(e.what());
    }

    // Add the Frame Ready channel to the reactor
    reactor_->register_channel(rxChannel_, boost::bind(&SharedMemoryController::handleRxChannel, this));

    // Now connect the frame release response channel
    try {
      LOG4CXX_DEBUG(logger_, "Connecting TX Channel to endpoint: " << txEndPoint);
      txChannel_.connect(txEndPoint.c_str());
    }
    catch (zmq::error_t& e) {
      //std::stringstream ss;
      //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
      // TODO: What to do here, I think throw it up
      throw std::runtime_error(e.what());
    }
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

  void SharedMemoryController::handleRxChannel()
  {
    // Receive a message from the main thread channel
    std::string rxMsgEncoded = rxChannel_.recv();

    LOG4CXX_DEBUG(logger_, "RX thread called with message: " << rxMsgEncoded);

    // Parse and handle the message
    try {
      FrameReceiver::IpcMessage rxMsg(rxMsgEncoded.c_str());

      if ((rxMsg.get_msg_type() == FrameReceiver::IpcMessage::MsgTypeNotify) &&
          (rxMsg.get_msg_val()  == FrameReceiver::IpcMessage::MsgValNotifyFrameReady)){
        int bufferID = rxMsg.get_param<int>("buffer_id", -1);

        if (bufferID != -1){
          // Create a frame object and copy in the raw frame data
          boost::shared_ptr<Frame> frame;
          frame = boost::shared_ptr<Frame>(new Frame("raw"));
          smp_->get_frame((*frame), bufferID);
          // Set the frame number
          frame->set_frame_number(rxMsg.get_param<int>("frame", 0));

          // Loop over registered callbacks, placing the frame onto each queue
          std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
          for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
            cbIter->second->getWorkQueue()->add(frame);
          }

          // We want to send the notify frame release message back to the frameReceiver
          FrameReceiver::IpcMessage txMsg(FrameReceiver::IpcMessage::MsgTypeNotify,
                                          FrameReceiver::IpcMessage::MsgValNotifyFrameRelease);

          txMsg.set_param("frame", rxMsg.get_param<int>("frame"));
          txMsg.set_param("buffer_id", rxMsg.get_param<int>("buffer_id"));
          LOG4CXX_DEBUG(logger_, "Sending response: " << txMsg.encode());

          // Now publish the release message, to notify the frame receiver that we are
          // finished with that block of shared memory
          txChannel_.send(txMsg.encode());

        } else {
          LOG4CXX_ERROR(logger_, "RX thread received empty frame notification with buffer ID");
        }
      } else {
        LOG4CXX_ERROR(logger_, "RX thread got unexpected message: " << rxMsgEncoded);

        //IpcMessage rx_reply;

        //rx_reply.set_msg_type(IpcMessage::MsgTypeNack);
        //rx_reply.set_msg_val(rx_msg.get_msg_val());
        //TODO add error in params

        //rx_channel_.send(rx_reply.encode());
      }
    }
    catch (FrameReceiver::IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
    }
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
