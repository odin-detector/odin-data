/*!
 * FrameReceiverRxThread.cpp - abstract receiver thread base class for the FrameReceiver application
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverRxThread.h"

using namespace FrameReceiver;

//! Constructor for the FrameReceiverRxThread class.
//!
//! This constructor initialises the member variables of the class. Startup of the thread
//! itself is deferred to the start() method.
//!
FrameReceiverRxThread::FrameReceiverRxThread(FrameReceiverConfig& config,
                                             SharedBufferManagerPtr buffer_manager,
                                             FrameDecoderPtr frame_decoder,
                                             unsigned int tick_period_ms) :
    config_(config),
    logger_(log4cxx::Logger::getLogger("FR.RxThread")),
    buffer_manager_(buffer_manager),
    frame_decoder_(frame_decoder),
    tick_period_ms_(tick_period_ms),
    rx_channel_(ZMQ_DEALER),
    run_thread_(true),
    thread_running_(false),
    thread_init_error_(false)
{
}

//! Destructor for the FrameReceiverRxThread.
//!
FrameReceiverRxThread::~FrameReceiverRxThread()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Destroying FrameReceiverRxThread....");
}

//! Start the FrameReceiverRxThread.
//!
//! This method starts the RX thread proper, blocking until the thread is started or has
//! signalled an initialisation error, in which event an exception is thrown.
//!
//! \return - bool indicating if thread initialisation and startup succeeded
//!
bool FrameReceiverRxThread::start()
{

  bool init_ok = true;

  rx_thread_ = boost::shared_ptr<boost::thread>(
    new boost::thread(boost::bind(&FrameReceiverRxThread::run_service, this)));

  // Wait for the thread service to initialise and be running properly, so that
  // this constructor only returns once the object is fully initialised (RAII).
  // Monitor the thread error flag, log an error and exit false if failed.

  while (!thread_running_)
  {
    if (thread_init_error_) {
      run_thread_ = false;
      LOG4CXX_ERROR(logger_, "RX thread initialisation failed: " << thread_init_msg_);
      init_ok = false;
      break;
    }
  }

  return init_ok;
}

//! Stop the FrameReceiverRxThread.
//!
//! This method stops the receiver thread, signalling for it to come to a controlled stop
//! and waiting for the thead to join. Any cleanup of the specialised thread service type
//! is also called.
//!
void FrameReceiverRxThread::stop()
{
  run_thread_ = false;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Waiting for RX thread to stop....");
  if (rx_thread_){
    rx_thread_->join();
  }
  LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread stopped....");

  // Run the specific service cleanup implemented in subclass
  cleanup_specific_service();
}

//! Run the RX thread event loop service
//!
//! This method is the entry point for the RX thread and, having configured message channels
//! and timers, runs the IPC reactor event loop, which blocks during normal operation. Once the
//! reactor exits, the necessary cleanup of the thread is performed.
//!
void FrameReceiverRxThread::run_service(void)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running RX thread service");

  // Configure thread-specific logging parameters
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());

  // Connect the message channel to the main thread
  try
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting RX channel to endpoint "
      << config_.rx_channel_endpoint_);
    rx_channel_.connect(config_.rx_channel_endpoint_);
  }
  catch (zmq::error_t& e)
  {
    std::stringstream ss;
    ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ 
      << " failed: " << e.what();
    this->set_thread_init_error(ss.str());
    return;
  }

  // Add the RX channel to the reactor
  reactor_.register_channel(rx_channel_, 
    boost::bind(&FrameReceiverRxThread::handle_rx_channel, this));

  // Run the specific service setup implemented in subclass
  run_specific_service();

  // Add the tick timer to the reactor
  int tick_timer_id = reactor_.register_timer(tick_period_ms_, 0, 
    boost::bind(&FrameReceiverRxThread::tick_timer, this));

  // Add the buffer monitor timer to the reactor
  int buffer_monitor_timer_id = reactor_.register_timer(frame_decoder_->get_frame_timeout_ms(), 0, 
    boost::bind(&FrameReceiverRxThread::buffer_monitor_timer, this));

  // Register the frame release callback with the decoder
  frame_decoder_->register_frame_ready_callback(
    boost::bind(&FrameReceiverRxThread::frame_ready, this, _1, _2));

  // If there was any prior error setting the thread up, return
  if (thread_init_error_)
  {
    return;
  }

  // Set thread state to running, allowing the start method in the main thread to return
  thread_running_ = true;

  // Advertise RX thread channel identity to the main thread so it knows how to route messages back
  this->advertise_identity();

  // Send a precharge request to the main thread if the frame decoder has no empty buffers queued
  if (frame_decoder_->get_num_empty_buffers() == 0) {
    this->request_buffer_precharge();
  }

  // Run the reactor event loop
  reactor_.run();

  // Cleanup - remove channels, sockets and timers from the reactor and close the receive socket
  reactor_.remove_channel(rx_channel_);
  reactor_.remove_timer(tick_timer_id);
  reactor_.remove_timer(buffer_monitor_timer_id);

  for (std::vector<int>::iterator recv_sock_it = recv_sockets_.begin(); 
        recv_sock_it != recv_sockets_.end(); recv_sock_it++)
  {
    reactor_.remove_socket(*recv_sock_it);
    close(*recv_sock_it);
  }
  recv_sockets_.clear();
  rx_channel_.close();

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Terminating RX thread service");

}

//! Advertise the RX thread channel identity.
//!
//! This method advertises the identity of the RX thread endpoint on the channel 
//! communicating with the main thread. This allows the main thread to correctly route
//! messages to the RX thread over the ROUTER-DEALER channel.
//!
void FrameReceiverRxThread::advertise_identity(void)
{
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Advertising RX thread identity");
  IpcMessage identity_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyIdentity);

  rx_channel_.send(identity_msg.encode());
}

//! Request frame buffer precharge.
//!
//! This method requests precharge of the empty buffer queue from the main thread, allowing
//! it to fill the empty buffer queue prior to receiving any frame data.
//!
void FrameReceiverRxThread::request_buffer_precharge(void)
{

  LOG4CXX_DEBUG_LEVEL(3, logger_, "Requesting buffer precharge");
  IpcMessage precharge_msg(IpcMessage::MsgTypeCmd, IpcMessage::MsgValCmdBufferPrechargeRequest);

  rx_channel_.send(precharge_msg.encode());
}

//! Handle messages on the RX channel.
//!
//! This method is the handler registered with the thread reactor to handle incoming messages on
//! the RX channel to the main thread.
//!
void FrameReceiverRxThread::handle_rx_channel(void)
{
  // Receive a message from the main thread channel
  std::string rx_msg_encoded = rx_channel_.recv();

  // Decode the message and handle appropriately
  try {

    IpcMessage rx_msg(rx_msg_encoded.c_str());
    IpcMessage::MsgType msg_type = rx_msg.get_msg_type();
    IpcMessage::MsgVal msg_val = rx_msg.get_msg_val();

    IpcMessage rx_reply;

    switch (msg_type)
    {

      // Handle command messages
      case IpcMessage::MsgTypeCmd:

        switch (msg_val)
        {

          case IpcMessage::MsgValCmdStatus:
            rx_reply.set_msg_type(IpcMessage::MsgTypeAck);
            rx_reply.set_msg_val(IpcMessage::MsgValCmdStatus);
            this->fill_status_params(rx_reply);
            rx_channel_.send(rx_reply.encode());
            break;

          default:
            LOG4CXX_ERROR(logger_,
              "Got unexpected value on command message from main thread: " << rx_msg_encoded);
            break;
        }
        break;

      // Handle acknowledgement messages
      case IpcMessage::MsgTypeAck:

        switch (msg_val)
        {

        case IpcMessage::MsgValNotifyIdentity:
          LOG4CXX_DEBUG_LEVEL(3, logger_, 
            "RX thread received acknowledgement of identity notification");
          break;
        
        default:
          LOG4CXX_ERROR(logger_,
            "Got unexpected value on acknowlege message from main thread: " << rx_msg_encoded);
          break;
        }
        break;

      // Handle notification messages
      case IpcMessage::MsgTypeNotify:

        switch (msg_val)
        {
        case IpcMessage::MsgValNotifyBufferPrecharge:
          {
            int start_buffer_id = rx_msg.get_param<int>("start_buffer_id", -1);
            int num_buffers = rx_msg.get_param<int>("num_buffers", -1);

            if ((start_buffer_id == -1) || (num_buffers == -1))
            {
              LOG4CXX_ERROR(logger_, "RX thread received precharge notification with missing buffer parameters");
            }
            else
            {
              for (int buffer_id = start_buffer_id; buffer_id < start_buffer_id + num_buffers; buffer_id++)
              {
                frame_decoder_->push_empty_buffer(buffer_id);
              }
              LOG4CXX_DEBUG_LEVEL(1, logger_, "Precharged " << num_buffers << " empty buffers onto queue, "
                << "length is now " << frame_decoder_->get_num_empty_buffers());
            }
          }
          break;

        case IpcMessage::MsgValNotifyFrameRelease:
          {
            
            int buffer_id = rx_msg.get_param<int>("buffer_id", -1);      
            if (buffer_id != -1)
            {
              frame_decoder_->push_empty_buffer(buffer_id);
              LOG4CXX_DEBUG_LEVEL(3, logger_, "Added empty buffer ID " << buffer_id 
                << " to queue, length is now " << frame_decoder_->get_num_empty_buffers());
            }
            else
            {
              LOG4CXX_ERROR(logger_, "RX thread received empty frame notification with buffer ID");
            }
          }
          break;

        default:
          LOG4CXX_ERROR(logger_,
            "Got unexpected value on notify message from main thread: " << rx_msg_encoded);
          break;
        }
        break;

      default:
        LOG4CXX_ERROR(logger_, 
          "Got unexpected type on message from main thread: " << rx_msg_encoded);      
        IpcMessage rx_reply;          
        rx_reply.set_msg_type(IpcMessage::MsgTypeNack);
        rx_reply.set_msg_val(msg_val);
        rx_reply.set_param("error", std::string("Unexpected message type from main thread"));
        rx_channel_.send(rx_reply.encode());
        break;
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding RX channel request: " << e.what());
  }
}

//! Tick timer handler for the RX thread.
//!
//! This method is the tick timer handler for the RX thread and is called periodically
//! by the thread reactor event loop. Its purpose is to detect external requests to stop
//! the thread via the stop() method, which clears the run_thread condition.
//!
void FrameReceiverRxThread::tick_timer(void)
{
  LOG4CXX_DEBUG_LEVEL(4, logger_, "RX thread tick timer fired");
  if (!run_thread_)
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread terminate detected in timer");
    reactor_.stop();
  }
}

//! Buffer monitor timer handler for the RX thread.
//!
//! This method is the buffer monitor timer handler for the RX thread and is called periodically
//! by the thread reactor event loop. It calls the frame decoder buffer montoring function to
//! allow, e.g. timed out frames to be released. It also sends a status notification to the main
//! thread to allow status information to be updated ready for client requests.
//!
void FrameReceiverRxThread::buffer_monitor_timer(void)
{
  LOG4CXX_DEBUG_LEVEL(4, logger_, "RX thread buffer monitor thread fired");
  frame_decoder_->monitor_buffers();

  // Send status notification to main thread
  IpcMessage status_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyStatus);
  this->fill_status_params(status_msg);

  rx_channel_.send(status_msg.encode());
}

//! Fill status parameters into a message.
//! 
//! This method populates the parameter block of the IpcMessage passed as an argument
//! with the current state of various RX thread and frame decoder values. This is used
//! to build status messages for communication with the main thread.
//!
//! \param[in,out] status_msg - IpcMessage to fill with status parameters
//!
void FrameReceiverRxThread::fill_status_params(IpcMessage& status_msg)
{
  status_msg.set_param("rx_thread/empty_buffers", frame_decoder_->get_num_empty_buffers());
  status_msg.set_param("rx_thread/mapped_buffers", frame_decoder_->get_num_mapped_buffers());
  status_msg.set_param("rx_thread/frames_timedout", frame_decoder_->get_num_frames_timedout());

  // Get the specific frame decoder instance to fill its own status into message
  frame_decoder_->get_status(std::string("decoder/"), status_msg);

}

//! Signal that a frame is ready for processing.
//!
//! This method is called to signal to the main thread that a frame is ready (either complete or
//! timed out) for processing by the downstream application. An IpcMessage is created with
//! the appropriate parameters and passed to the amin thread via the RX channel.
//!
//! \param[in] buffer_id - buffer manager ID that is ready
//! \param[in] frame_number - frame number contained in that buffer
//!
void FrameReceiverRxThread::frame_ready(int buffer_id, int frame_number)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Releasing frame " << frame_number << " in buffer " << buffer_id);

  IpcMessage ready_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameReady);
  ready_msg.set_param("frame", frame_number);
  ready_msg.set_param("buffer_id", buffer_id);

  rx_channel_.send(ready_msg.encode());

}

//! Set thread initialisation error condition.
//!
//! This method is called by the RX thread initialisation to indicate that an error has
//! occurred. The error message passed as an argument is stored so it can be retrieved
//! by the calling process as appropriate.
//!
//! \param[in] msg - error message associated with initialisation
//!
void FrameReceiverRxThread::set_thread_init_error(const std::string& msg)
{
  thread_init_msg_ = msg;
  thread_init_error_ = true;
}

//! Register a socket with the RX thread reactor
//!
//! This method registers a socket and associated callback with the RX thread reactor
//! instance, to allow data arriving on that socket to be handled appropriately
//!
//! \param[in] socket_fd - file descriptor of socket to register
//! \param[in] callback - callback function to be called by reactor on this socket
//!
void FrameReceiverRxThread::register_socket(int socket_fd, ReactorCallback callback)
{
  // Add the receive socket to the reactor
  reactor_.register_socket(socket_fd, callback);

  recv_sockets_.push_back(socket_fd);
}
