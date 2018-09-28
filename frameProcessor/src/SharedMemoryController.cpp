/*
 * SharedMemoryController.cpp
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#include <SharedMemoryController.h>
#include "DebugLevelLogger.h"

namespace FrameProcessor
{

const std::string SharedMemoryController::SHARED_MEMORY_CONTROLLER_NAME = "shared_memory";

/** Constructor.
 *
 * The constructor sets up logging used within the class. It also creates the
 * rxChannel_ subscribing IpcChannel and registers it with the IpcReactor reactor_.
 * Finally, the txChannel_ publishing IpcChannel is created. rxChannel_ is a ZeroMQ
 * subscriber (listening for frame ready notifications from the frame recevier) and
 * txChannel_ is a ZeroMQ plublisher that sends notifications (of released frames to
 * the frame receiver).
 *
 * \param[in] reactor - pointer to the IpcReactor object.
 * \param[in] rxEndPoint - string name of the subscribing endpoint for frame ready notifications.
 * \param[in] txEndPoint - string name of the publishing endpoint for frame release notifications.
 */
SharedMemoryController::SharedMemoryController(boost::shared_ptr<OdinData::IpcReactor> reactor,
                                               const std::string& rxEndPoint,
                                               const std::string& txEndPoint) :
    reactor_(reactor),
    rxChannel_(ZMQ_SUB),
    txChannel_(ZMQ_PUB),
    sharedBufferConfigured_(false)
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FW.SharedMemoryController");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "SharedMemoryController constructor.");

  // Connect the frame ready channel
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting RX Channel to endpoint: " << rxEndPoint);
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
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting TX Channel to endpoint: " << txEndPoint);
    txChannel_.connect(txEndPoint.c_str());
  }
  catch (zmq::error_t& e) {
    //std::stringstream ss;
    //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
    // TODO: What to do here, I think throw it up
    throw std::runtime_error(e.what());
  }

  // Request the shared buffer configuration from the upstream frame receiver process. This is deferred
  // with a one-shot reactor timer to allow the background channel conneciton to take place.
  this->requestSharedBufferConfig(true);
}

/**
 * Destructor.
 */
SharedMemoryController::~SharedMemoryController()
{
  LOG4CXX_TRACE(logger_, "Shutting down SharedMemoryController");
  // Close the IPC Channels
  reactor_->remove_channel(txChannel_);
  reactor_->remove_channel(rxChannel_);
  txChannel_.close();
  rxChannel_.close();
}

/** setSharedBufferManager
 * Takes a name of shared buffer manager and initialises a SharedBufferManager object
 *
 * \param[in] shared_buffer_name - name of the shared buffer manager
 */
void SharedMemoryController::setSharedBufferManager(const std::string& shared_buffer_name)
{
  // Reset the shared buffer manager if already existing
  if (sbm_) {
    sbm_.reset();
  }

  // Create a new shared buffer manager
  sbm_ =  boost::shared_ptr<OdinData::SharedBufferManager>(
      new OdinData::SharedBufferManager(shared_buffer_name)
  );

  sharedBufferConfigured_ = true;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Initialised shared buffer manager for buffer " << shared_buffer_name);
}

/** Request the shared buffer configuration information from the upstream frame receiver process
 *
 * Requests the name of the shared buffer manager from the upstream frame receiver process by
 * sending an IpcMessage over the frame notification channel. This can be deferred to allow
 * time for the notification channels to be connected, using a single-shot timer registered on
 * the reactor.
 *
 * \param[in] deferred - true if the request should be deferred
 */
void SharedMemoryController::requestSharedBufferConfig(const bool deferred)
{
  if (deferred)
  {
    reactor_->register_timer(1000, 1, boost::bind(&SharedMemoryController::requestSharedBufferConfig, this, false));
  }
  else
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Requesting shared buffer configuration from frame receiver");

    OdinData::IpcMessage config_request(OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdBufferConfigRequest);

    txChannel_.send(config_request.encode());
  }
}

/** Called whenever a new IpcMessage is received to notify that a frame is ready.
 *
 * Reads the raw message bytes from the rxChannel_ and constructs an IpcMessage object
 * from the bytes. Verifies the IpcMessage type and value, and then uses the frame
 * number and buffer ID information to tell the SharedMemoryParser which frame is ready
 * for extraction from shared memory.
 * Loops over registered callbacks and passes the frame to the relevant WorkQueue objects,
 * before sending notifiation that the frame has been released for re-use.
 */
void SharedMemoryController::handleRxChannel()
{
  // Receive a message from the main thread channel
  std::string rxMsgEncoded = rxChannel_.recv();

  LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread called with message: " << rxMsgEncoded);

  // Parse and handle the message
  try {
    OdinData::IpcMessage rxMsg(rxMsgEncoded.c_str());

    if ((rxMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeNotify) &&
        (rxMsg.get_msg_val()  == OdinData::IpcMessage::MsgValNotifyFrameReady)) {

      int bufferID = rxMsg.get_param<int>("buffer_id", -1);
      if (bufferID != -1) {

        if (sbm_) {

          // Create a frame object and copy in the raw frame data
          boost::shared_ptr<Frame> frame;
          frame = boost::shared_ptr<Frame>(new Frame("raw"));
          frame->wrap_shared_buffer(sbm_->get_buffer_address(bufferID),
                                    sbm_->get_buffer_size(),
                                    bufferID,rxMsg.get_param<int>("frame", 0),
                                    &txChannel_);

          // Set the frame number
          frame->set_frame_number(rxMsg.get_param<int>("frame", 0));

          // Loop over registered callbacks, placing the frame onto each queue
          std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
          for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter) {
            cbIter->second->getWorkQueue()->add(frame, true);
          }

        } else {
          LOG4CXX_WARN(logger_, "RX thread got notification for buffer " << bufferID
                         << " with no shared buffer config - ignoring");
        }

      } else {
        LOG4CXX_ERROR(logger_, "RX thread received empty frame notification with buffer ID");
      }
    } else if ((rxMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeNotify) &&
               (rxMsg.get_msg_val()  == OdinData::IpcMessage::MsgValNotifyBufferConfig))
    {
      try
      {
        std::string shared_buffer_name = rxMsg.get_param<std::string>("shared_buffer_name");
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Shared buffer config notification received for " << shared_buffer_name);
        this->setSharedBufferManager(shared_buffer_name);
      }
      catch (OdinData::IpcMessageException& e)
      {
        LOG4CXX_ERROR(logger_, "Received shared buffer config notification with no name parameter");
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
  catch (OdinData::IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
  }
}

/** Register a callback for Frame updates with this class.
 *
 * The callback (IFrameCallback subclass) is added to the map of callbacks, indexed
 * by name. Whenever a new Frame object is received from the frame receiver then these
 * callbacks will be called and passed the Frame pointer.
 *
 * \param[in] name - string index of the callback.
 * \param[in] cb - IFrameCallback to register for updates.
 */
void SharedMemoryController::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb)
{
  // Check if we own the callback already
  if (callbacks_.count(name) == 0) {
    // Record the callback pointer
    callbacks_[name] = cb;
    // Confirm registration
    cb->confirmRegistration("frame_receiver");
  }
}

/** Remove a callback from the callback map.
 *
 * The callback is removed from the map of callbacks.
 *
 * \param[in] name - string index of the callback.
 */
void SharedMemoryController::removeCallback(const std::string& name)
{
  boost::shared_ptr<IFrameCallback> cb;
  if (callbacks_.count(name) > 0) {
    // Get the pointer
    cb = callbacks_[name];
    // Remove the callback from the map
    callbacks_.erase(name);
    // Confirm removal
    cb->confirmRemoval("frame_receiver");
  }
}

/**
 * Collate status information for the plugin. The status is added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the status.
 */
void SharedMemoryController::status(OdinData::IpcMessage& status)
{
  // Set status parameters in the status message
  status.set_param(
      SharedMemoryController::SHARED_MEMORY_CONTROLLER_NAME + "/configured",
      sharedBufferConfigured_);

}

} /* namespace FrameProcessor */
