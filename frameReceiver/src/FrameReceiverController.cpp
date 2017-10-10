/*
 * FrameReceiverController.cpp - controller class for the FrameReceiver application
 *
 *  Created on: Oct 10, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverController.h"

using namespace FrameReceiver;

bool FrameReceiverController::terminate_controller_ = false;

#ifndef BUILD_DIR
#define BUILD_DIR "."
#endif

FrameReceiverController::FrameReceiverController () :
    logger_(log4cxx::Logger::getLogger("FR.Controller")),
    rx_channel_(ZMQ_PAIR),
    ctrl_channel_(ZMQ_ROUTER),
    frame_ready_channel_(ZMQ_PUB),
    frame_release_channel_(ZMQ_SUB),
    frames_received_(0),
    frames_released_(0)
{
  LOG4CXX_TRACE(logger_, "FrameRecevierController constructor");

}

//! Destructor for the FrameReceiverController
FrameReceiverController::~FrameReceiverController ()
{

  // Delete the RX thread object by resetting the scoped pointer, allowing the IPC channel
  // to be closed cleanly
  rx_thread_.reset();

}

void FrameReceiverController::configure(FrameReceiverConfig& config, OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply)
{
  LOG4CXX_DEBUG(logger_, "Configuration submitted: " << config_msg.encode());
  config_ = config; // TODO REMOVE THIS!
}

void FrameReceiverController::run(void)
{
  LOG4CXX_TRACE(logger_, "FrameReceiverController::run()");
  terminate_controller_ = false;

  // Initialise IPC channels
  initialise_ipc_channels();

  // Create the appropriate frame decoder
  initialise_frame_decoder();

  // Initialise the frame buffer buffer manager
  initialise_buffer_manager();

  // Create the RX thread object
  switch(config_.rx_type_)
  {
    case Defaults::RxTypeUDP:
      rx_thread_.reset(new FrameReceiverUDPRxThread( config_, logger_, buffer_manager_, frame_decoder_));
      break;

    case Defaults::RxTypeZMQ:
      rx_thread_.reset(new FrameReceiverZMQRxThread( config_, logger_, buffer_manager_, frame_decoder_));
      break;

    default:
      throw OdinData::OdinDataException("Cannot create RX thread - RX type not recognised");
  }
  rx_thread_->start();

  // Pre-charge all frame buffers onto the RX thread queue ready for use
  precharge_buffers();

  // Notify downstream processes of current buffer configuration
  notify_buffer_config(true);

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Main thread entering reactor loop");

  // Run the reactor event loop
  reactor_.run();

  // Call cleanup on the RxThread
  rx_thread_->stop();

  // Destroy the RX thread
  rx_thread_.reset();

  // Clean up IPC channels
  cleanup_ipc_channels();

  // Remove timers and channels from the reactor
  //reactor_.remove_timer(rxPingTimer);
  //reactor_.remove_timer(timer2);

}

void FrameReceiverController::stop(void)
{
  LOG4CXX_TRACE(logger_, "FrameReceiverController::stop()");
  terminate_controller_ = true;
}


void FrameReceiverController::initialise_ipc_channels(void)
{
  // Bind the control channel
  ctrl_channel_.bind(config_.ctrl_channel_endpoint_);

  // Bind the RX thread channel
  rx_channel_.bind(config_.rx_channel_endpoint_);

  // Bind the frame ready and release channels
  try {
    frame_ready_channel_.bind(config_.frame_ready_endpoint_);
    frame_release_channel_.bind(config_.frame_release_endpoint_);
  }
  catch (std::exception& e)
  {
    LOG4CXX_ERROR(logger_, "Got exception binding IPC channels: " << e.what());
  }

  // Set default subscription on frame release channel
  frame_release_channel_.subscribe("");

  // Add IPC channels to the reactor
  reactor_.register_channel(ctrl_channel_, boost::bind(&FrameReceiverController::handle_ctrl_channel, this));
  reactor_.register_channel(rx_channel_, boost::bind(&FrameReceiverController::handle_rx_channel, this));
  reactor_.register_channel(frame_release_channel_, boost::bind(&FrameReceiverController::handle_frame_release_channel, this));

}

void FrameReceiverController::cleanup_ipc_channels(void)
{
  // Remove IPC channels from the reactor
  reactor_.remove_channel(ctrl_channel_);
  reactor_.remove_channel(rx_channel_);
  reactor_.remove_channel(frame_release_channel_);

  // Close all channels
  ctrl_channel_.close();
  rx_channel_.close();
  frame_ready_channel_.close();
  frame_release_channel_.close();

}

void FrameReceiverController::initialise_frame_decoder(void)
{
  std::string libDir(BUILD_DIR);
  libDir += "/lib/";
  if (config_.sensor_path_ != ""){
    libDir = config_.sensor_path_;
    // Check if the last character is '/', if not append it
    if (*libDir.rbegin() != '/'){
      libDir += "/";
    }
  }
  std::string libName = "lib" + config_.sensor_type_ + "FrameDecoder.so";
  std::string clsName = config_.sensor_type_ + "FrameDecoder";
  LOG4CXX_INFO(logger_, "Loading decoder plugin " << clsName << " from " << libDir << libName);

  try {
    frame_decoder_ = OdinData::ClassLoader<FrameDecoder>::load_class(clsName, libDir + libName);
    if (!frame_decoder_){
      throw OdinData::OdinDataException("Cannot initialise frame decoder: sensor type not recognised");
    } else {
      LOG4CXX_INFO(logger_, "Created " << clsName << " frame decoder instance");
    }
  }
  catch (const std::runtime_error& e) {
    std::stringstream sstr;
    sstr << "Cannot initialise frame decoder: " << e.what();
    throw OdinData::OdinDataException(sstr.str());
  }
  // Initialise the decoder object
  frame_decoder_->init(logger_, config_.enable_packet_logging_, config_.frame_timeout_ms_);
}


void FrameReceiverController::initialise_buffer_manager(void)
{
  // Create a shared buffer manager
  buffer_manager_.reset(new SharedBufferManager(config_.shared_buffer_name_, config_.max_buffer_mem_,
                                                frame_decoder_->get_frame_buffer_size(), false));
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Initialised frame buffer manager of total size " << config_.max_buffer_mem_ <<
                                                                                    " with " << buffer_manager_->get_num_buffers() << " buffers");

  // Register buffer manager with the frame decoder
  frame_decoder_->register_buffer_manager(buffer_manager_);

}

void FrameReceiverController::precharge_buffers(void)
{
  // Push the IDs of all of the empty buffers onto the RX thread channel
  // TODO if the number of buffers is so big that the RX thread channel would reach HWM (in either direction)
  // before the reactor has time to start, we could consider putting this pre-charge into a timer handler
  // that runs as soon as the reactor starts, but need to think about how this might block. Need non-blocking
  // send on channels??
  for (int buf = 0; buf < buffer_manager_->get_num_buffers(); buf++)
  {
    IpcMessage buf_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameRelease);
    buf_msg.set_param("buffer_id", buf);
    rx_channel_.send(buf_msg.encode());
  }
}

void FrameReceiverController::notify_buffer_config(const bool deferred)
{
  // Notify downstream applications listening on the frame ready channel of the current shared buffer
  // configuration.

  if (deferred)
  {
    reactor_.register_timer(1000, 1, boost::bind(&FrameReceiverController::notify_buffer_config, this, false));
  }
  else
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Notifying downstream processes of shared buffer configuration");

    IpcMessage config_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyBufferConfig);
    config_msg.set_param("shared_buffer_name", config_.shared_buffer_name_);

    frame_ready_channel_.send(config_msg.encode());
  }
}

void FrameReceiverController::handle_ctrl_channel(void)
{
  // Receive a request message from the control channel
  std::string client_identity;
  std::string ctrl_req_encoded = ctrl_channel_.recv(&client_identity);

  // Construct a default reply
  IpcMessage ctrl_reply;

  // Parse and handle the message
  try {

    IpcMessage ctrl_req(ctrl_req_encoded.c_str());

    switch (ctrl_req.get_msg_type())
    {
      case IpcMessage::MsgTypeCmd:

      LOG4CXX_DEBUG_LEVEL(3, logger_, "Got control channel command request from client " << client_identity);
        ctrl_reply.set_msg_type(IpcMessage::MsgTypeAck);
        ctrl_reply.set_msg_val(ctrl_req.get_msg_val());
        break;

      default:
      LOG4CXX_ERROR(logger_, "Got unexpected command on control channel with type " << ctrl_req.get_msg_type());
        ctrl_reply.set_msg_type(IpcMessage::MsgTypeNack);
        ctrl_reply.set_msg_val(ctrl_req.get_msg_val());
        break;
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
  }
  ctrl_channel_.send(ctrl_reply.encode(), 0, client_identity);

}

void FrameReceiverController::handle_rx_channel(void)
{
  std::string rx_reply_encoded = rx_channel_.recv();
  try {
    IpcMessage rx_reply(rx_reply_encoded.c_str());
    //LOG4CXX_DEBUG_LEVEL(1, logger_, "Got reply from RX thread : " << rx_reply_encoded);

    if ((rx_reply.get_msg_type() == IpcMessage::MsgTypeNotify) &&
        (rx_reply.get_msg_val() == IpcMessage::MsgValNotifyFrameReady))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame ready notification from RX thread"
          " for frame " << rx_reply.get_param<int>("frame", -1) <<
                        " in buffer " << rx_reply.get_param<int>("buffer_id", -1));
      frame_ready_channel_.send(rx_reply_encoded);

      frames_received_++;
    }
    else
    {
      LOG4CXX_ERROR(logger_, "Got unexpected message from RX thread: " << rx_reply_encoded);
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding RX thread channel reply: " << e.what());
  }
}

void FrameReceiverController::handle_frame_release_channel(void)
{
  std::string frame_release_encoded = frame_release_channel_.recv();
  try {
    IpcMessage frame_release(frame_release_encoded.c_str());
    //LOG4CXX_DEBUG(logger_, "Got message on frame release channel : " << frame_release_encoded);

    if ((frame_release.get_msg_type() == IpcMessage::MsgTypeNotify) &&
        (frame_release.get_msg_val() == IpcMessage::MsgValNotifyFrameRelease))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame release notification from processor"
          " from frame " << frame_release.get_param<int>("frame", -1) <<
                         " in buffer " << frame_release.get_param<int>("buffer_id", -1));
      rx_channel_.send(frame_release_encoded);

      frames_released_++;

      if (config_.frame_count_ && (frames_released_ >= config_.frame_count_))
      {
        LOG4CXX_INFO(logger_, "Specified number of frames (" << config_.frame_count_ << ") received and released, terminating");
        stop();
        reactor_.stop();
      }
    }
    else if ((frame_release.get_msg_type() == IpcMessage::MsgTypeCmd) &&
             (frame_release.get_msg_val() == IpcMessage::MsgValCmdBufferConfigRequest))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got shared buffer config request from processor");
      notify_buffer_config(false);
    }
    else
    {
      LOG4CXX_ERROR(logger_, "Got unexpected message on frame release channel: " << frame_release_encoded);
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding message on frame release channel: " << e.what());
  }
}
