/*!
 * FrameReceiverRxThread.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverRxThread.h"

using namespace FrameReceiver;

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
    recv_socket_(0),
    run_thread_(true),
    thread_running_(false),
    thread_init_error_(false)
{
}

FrameReceiverRxThread::~FrameReceiverRxThread()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Destroying FrameReceiverRxThread....");
}

void FrameReceiverRxThread::start()
{
  rx_thread_ = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&FrameReceiverRxThread::run_service, this)));

  // Wait for the thread service to initialise and be running properly, so that
  // this constructor only returns once the object is fully initialised (RAII).
  // Monitor the thread error flag and throw an exception if initialisation fails

  while (!thread_running_)
  {
    if (thread_init_error_) {
      rx_thread_->join();
      throw OdinData::OdinDataException(thread_init_msg_);
    }
  }
}

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

void FrameReceiverRxThread::run_service(void)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running RX thread service");

  // Connect the message channel to the main thread
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting RX channel to endpoint " << config_.rx_channel_endpoint_);
    rx_channel_.connect(config_.rx_channel_endpoint_);
  }
  catch (zmq::error_t& e) {
    std::stringstream ss;
    ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
    thread_init_msg_ = ss.str();
    thread_init_error_ = true;
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

  // Set thread state to running, allows constructor to return
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

void FrameReceiverRxThread::advertise_identity(void)
{
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Advertising RX thread identity");
  IpcMessage identity_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyIdentity);

  rx_channel_.send(identity_msg.encode());
}

void FrameReceiverRxThread::request_buffer_precharge(void)
{

  LOG4CXX_DEBUG_LEVEL(3, logger_, "Requesting buffer precharge");
  IpcMessage precharge_msg(IpcMessage::MsgTypeCmd, IpcMessage::MsgValCmdBufferPrechargeRequest);

  rx_channel_.send(precharge_msg.encode());
}

void FrameReceiverRxThread::handle_rx_channel(void)
{
  // Receive a message from the main thread channel
  std::string rx_msg_encoded = rx_channel_.recv();

  // Parse and handle the message
  try {

    IpcMessage rx_msg(rx_msg_encoded.c_str());
    IpcMessage::MsgType msg_type = rx_msg.get_msg_type();
    IpcMessage::MsgVal msg_val = rx_msg.get_msg_val();

    if ((msg_type == IpcMessage::MsgTypeAck) && (msg_val == IpcMessage::MsgValNotifyIdentity))
    {
      LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received acknowledgement of identity notification");
    }
    else if ((msg_type == IpcMessage::MsgTypeNotify) && (msg_val == IpcMessage::MsgValNotifyFrameRelease))
    {

      int buffer_id = rx_msg.get_param<int>("buffer_id", -1);

      if (buffer_id != -1)
      {
        frame_decoder_->push_empty_buffer(buffer_id);
        LOG4CXX_DEBUG_LEVEL(3, logger_, "Added empty buffer ID " << buffer_id << " to queue, "
            "length is now " << frame_decoder_->get_num_empty_buffers());
      }
      else
      {
        LOG4CXX_ERROR(logger_, "RX thread received empty frame notification with buffer ID");
      }

    }
    else if ((msg_type == IpcMessage::MsgTypeCmd) && (msg_val == IpcMessage::MsgValCmdStatus))
    {
      IpcMessage rx_reply;

      rx_reply.set_msg_type(IpcMessage::MsgTypeAck);
      rx_reply.set_msg_val(IpcMessage::MsgValCmdStatus);
      rx_reply.set_param("count", rx_msg.get_param<int>("count", -1));

      rx_channel_.send(rx_reply.encode());
    }
    else
    {
      LOG4CXX_ERROR(logger_, "RX thread got unexpected message: " << rx_msg_encoded);

      IpcMessage rx_reply;

      rx_reply.set_msg_type(IpcMessage::MsgTypeNack);
      rx_reply.set_msg_val(msg_val);
      //TODO add error in params

      rx_channel_.send(rx_reply.encode());
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
  }
}

void FrameReceiverRxThread::tick_timer(void)
{
  //LOG4CXX_DEBUG_LEVEL(4, logger_, "RX thread tick timer fired");
  if (!run_thread_)
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread terminate detected in timer");
    reactor_.stop();
  }
}

void FrameReceiverRxThread::buffer_monitor_timer(void)
{
  LOG4CXX_DEBUG_LEVEL(4, logger_, "RX thread buffer monitor thread fired");
  frame_decoder_->monitor_buffers();

  // Send status notification to main thread
  IpcMessage status_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyStatus);
  status_msg.set_param("rx_thread/empty_buffers", frame_decoder_->get_num_empty_buffers());
  status_msg.set_param("rx_thread/mapped_buffers", frame_decoder_->get_num_mapped_buffers());
  status_msg.set_param("rx_thread/frames_timedout", frame_decoder_->get_num_frames_timedout());

  rx_channel_.send(status_msg.encode());
}

void FrameReceiverRxThread::frame_ready(int buffer_id, int frame_number)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Releasing frame " << frame_number << " in buffer " << buffer_id);

  IpcMessage ready_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameReady);
  ready_msg.set_param("frame", frame_number);
  ready_msg.set_param("buffer_id", buffer_id);

  rx_channel_.send(ready_msg.encode());

}

void FrameReceiverRxThread::set_thread_init_error(const std::string& msg)
{
  thread_init_msg_ = msg;
  thread_init_error_ = true;
}

void FrameReceiverRxThread::register_socket(int socket_fd, ReactorCallback callback)
{
  // Add the receive socket to the reactor
  reactor_.register_socket(socket_fd, callback);

  recv_sockets_.push_back(socket_fd);
}
