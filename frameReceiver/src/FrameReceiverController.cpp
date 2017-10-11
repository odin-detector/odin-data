/*
 * FrameReceiverController.cpp - controller class for the FrameReceiver application
 *
 *  Created on: Oct 10, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverController.h"

using namespace FrameReceiver;

#ifndef BUILD_DIR
#define BUILD_DIR "."
#endif

//! Constructor for the FrameReceiverController class.
//!
//! This constructor initialises the logger, IPC channels and state
//! of the controller. Configuration and running are deferred to the
//! configure() and run() methods respectively.
//!
FrameReceiverController::FrameReceiverController () :
    logger_(log4cxx::Logger::getLogger("FR.Controller")),
    terminate_controller_(false),
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
//!
FrameReceiverController::~FrameReceiverController ()
{

  // Delete the RX thread object by resetting the scoped pointer, allowing the IPC channel
  // to be closed cleanly
  rx_thread_.reset();

}

//! Configure the FrameReceiverController
//!
//! This method configures the controller based on configuration parameters received as
//! an IpcMessage. Depending on the parameters present in that message, IPC channels
//! the frame decoder, frame buffer manager and RX thread are conditionally configured.
//!
//! \param[in] config_msg - IpcMessage containing configuration parameters
//! `param[out] reply_msg - Reply IpcMessage indicating success or failure of actions.
//!
void FrameReceiverController::configure(FrameReceiverConfig& config,
    OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Configuration submitted: " << config_msg.encode());
  config_ = config; // TODO REMOVE THIS!

  try {

    // Configure IPC channels
    this->configure_ipc_channels(config_msg);

    // Configure the appropriate frame decoder
    this->configure_frame_decoder(config_msg);

  }
  catch (FrameReceiverException& e) {
    LOG4CXX_ERROR(logger_, "Configuration error: " << e.what());
    config_reply.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
    config_reply.set_param<std::string>("error", std::string(e.what()));
  }
}

//! Run the FrameReceiverController
//!
//! This method runs the FrameReceiverController
void FrameReceiverController::run(void)
{
  LOG4CXX_TRACE(logger_, "FrameReceiverController::run()");
  terminate_controller_ = false;

  // Initialise IPC channels
  //configure_ipc_channels();

  // Create the appropriate frame decoder
  // configure_frame_decoder();

  // Initialise the frame buffer buffer manager
  initialise_buffer_manager();

  // Create the RX thread object
  switch(config_.rx_type_)
  {
    case Defaults::RxTypeUDP:
      rx_thread_.reset(new FrameReceiverUDPRxThread( config_, buffer_manager_, frame_decoder_));
      break;

    case Defaults::RxTypeZMQ:
      rx_thread_.reset(new FrameReceiverZMQRxThread( config_, buffer_manager_, frame_decoder_));
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

  // Destroy the frame decoder
  frame_decoder_.reset();

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
  reactor_.stop();
}


void FrameReceiverController::configure_ipc_channels(OdinData::IpcMessage& config_msg)
{
  // If a control endpoint is specified, bind the control channel
  if (config_msg.has_param(CONFIG_CTRL_ENDPOINT)) {
    std::string endpoint = config_msg.get_param<std::string>(CONFIG_CTRL_ENDPOINT);
    this->setup_control_channel(endpoint);
  }

  // If the endpoint is specified, bind the RX thread channel
  if (config_msg.has_param(CONFIG_RX_ENDPOINT)) {
    std::string endpoint = config_msg.get_param<std::string>(CONFIG_RX_ENDPOINT);
    this->setup_rx_channel(endpoint);
  }

  // If the endpoint is specified, bind the frame ready notification channel
  if (config_msg.has_param(CONFIG_FRAME_READY_ENDPOINT)) {
    std::string endpoint = config_msg.get_param<std::string>(CONFIG_FRAME_READY_ENDPOINT);
    this->setup_frame_ready_channel(endpoint);
  }

  // If the endpoint is specified, bind the frame release notification channel
  if (config_msg.has_param(CONFIG_FRAME_RELEASE_ENDPOINT)) {
    std::string endpoint = config_msg.get_param<std::string>(CONFIG_FRAME_RELEASE_ENDPOINT);
    this->setup_frame_release_channel(endpoint);

  }
}

//! Set up the control channel.
//!
//! This method sets up the control channel, binding to the specified endpoint and adding
//! the channel to the reactor.
//!
//! \param[in] control_endpoint - string URI of endpoint
//!
void FrameReceiverController::setup_control_channel(const std::string& endpoint)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Binding control channel to endpoint: " << endpoint);

  try {
    ctrl_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding control channel endpoint " << endpoint << " failed: " << e.what();
    throw FrameReceiverException(sstr.str());
  }

  // Add channel to the reactor
  reactor_.register_channel(ctrl_channel_, boost::bind(&FrameReceiverController::handle_ctrl_channel, this));

}

//! Set up the receiver thread channel.
//!
//! This method sets up the receiver thread channel, binding to the specified endpoint and adding
//! the channel to the reactor.
//!
//! \param[in] control_endpoint - string URI of endpoint
//!
void FrameReceiverController::setup_rx_channel(const std::string& endpoint)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Binding receiver thread channel to endpoint: " << endpoint);

  try {
    rx_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding receiver thread channel endpoint " << endpoint << " failed: " << e.what();
    throw FrameReceiverException(sstr.str());
  }

  // Add channel to the reactor
  reactor_.register_channel(rx_channel_, boost::bind(&FrameReceiverController::handle_rx_channel, this));

}

//! Set up the frame ready notification channel.
//!
//! This method sets up the frame ready notification, binding to the specified endpoint and adding
//! the channel to the reactor.
//!
//! \param[in] control_endpoint - string URI of endpoint
//!
void FrameReceiverController::setup_frame_ready_channel(const std::string& endpoint)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Binding frame ready notification channel to endpoint: " << endpoint);

  try {
    frame_ready_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding frame ready notification channel endpoint " << endpoint << " failed: " << e.what();
    throw FrameReceiverException(sstr.str());
  }

}

//! Set up the frame release notification channel.
//!
//! This method sets up the frame release notification, binding to the specified endpoint and adding
//! the channel to the reactor.
//!
//! \param[in] control_endpoint - string URI of endpoint
//!
void FrameReceiverController::setup_frame_release_channel(const std::string& endpoint)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Binding frame release notification channel to endpoint: " << endpoint);

  try {
    frame_release_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding frame release notification channel endpoint " << endpoint << " failed: " << e.what();
    throw FrameReceiverException(sstr.str());
  }

  // Set default subscription on frame release channel
  frame_release_channel_.subscribe("");

  // Add channel to the reactor
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

//! Configure the frame decoder
//!
//! This method configures a frame decoder by loading the resolving the appropriate
//! library and class based on the input configuration.
//!
//! \param[in] config_msg - IPC message containing configuration parameters
//!
void FrameReceiverController::configure_frame_decoder(OdinData::IpcMessage& config_msg)
{

  // Resolve the library path if specified in the config message, otherwise default
  // to the current BUILD_DIR parameter.
  std::string lib_dir(BUILD_DIR);
  lib_dir += "/lib/";

  if (config_msg.has_param(CONFIG_DECODER_PATH))
  {
    lib_dir = config_msg.get_param<std::string>(CONFIG_DECODER_PATH);

    // Check if the last character is '/', if not append it
    if (*lib_dir.rbegin() != '/')
    {
      lib_dir += "/";
    }
  }

  if (config_msg.has_param(CONFIG_DECODER_TYPE))
  {

    std::string decoder_type = config_msg.get_param<std::string>(CONFIG_DECODER_TYPE);
    if (decoder_type != Defaults::default_decoder_type)
    {
      std::string lib_name = "lib" + decoder_type + "FrameDecoder.so";
      std::string cls_name = decoder_type + "FrameDecoder";
      LOG4CXX_INFO(logger_, "Loading decoder plugin " << cls_name << " from " << lib_dir << lib_name);

      try {

        frame_decoder_ = OdinData::ClassLoader<FrameDecoder>::load_class(cls_name, lib_dir + lib_name);
        if (!frame_decoder_)
        {
          throw FrameReceiverException("Cannot configure frame decoder: plugin type not recognised");
        }
        else
        {
          LOG4CXX_INFO(logger_, "Created " << cls_name << " frame decoder instance");
        }
      }
      catch (const std::runtime_error& e) {
        std::stringstream sstr;
        sstr << "Cannot configure frame decoder: " << e.what();
        throw FrameReceiverException(sstr.str());
      }

      // Initialise the decoder object
      frame_decoder_->init(logger_, config_.enable_packet_logging_, config_.frame_timeout_ms_);
    }
    else
    {
      LOG4CXX_INFO(logger_, "No frame decoder loaded: type not specified");
    }
  }
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
