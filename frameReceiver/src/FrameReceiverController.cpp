/*
 * FrameReceiverController.cpp - controller class for the FrameReceiver application
 *
 *  Created on: Oct 10, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverController.h"

using namespace FrameReceiver;

#ifdef __APPLE__
#define SHARED_LIBRARY_SUFFIX ".dylib"
#else
#define SHARED_LIBRARY_SUFFIX ".so"
#endif

//! Constructor for the FrameReceiverController class.
//!
//! This constructor initialises the logger, IPC channels and state
//! of the controller. Configuration and running are deferred to the
//! configure() and run() methods respectively.
//!
FrameReceiverController::FrameReceiverController (FrameReceiverConfig& config) :
    logger_(log4cxx::Logger::getLogger("FR.Controller")),
    config_(config),
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
void FrameReceiverController::configure(OdinData::IpcMessage& config_msg,
    OdinData::IpcMessage& config_reply)
{

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Configuration submitted: " << config_msg.encode());

  config_reply.set_msg_val(config_msg.get_msg_val());

  try {

    // Configure IPC channels
    this->configure_ipc_channels(config_msg);

    // Configure the appropriate frame decoder
    this->configure_frame_decoder(config_msg);

    // Configure the buffer manager
    this->configure_buffer_manager(config_msg);

    // Configure the RX thread
    this->configure_rx_thread(config_msg);

    config_reply.set_msg_type(OdinData::IpcMessage::MsgTypeAck);

  }
  catch (FrameReceiverException& e) {
    LOG4CXX_ERROR(logger_, "Configuration error: " << e.what());
    config_reply.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
    config_reply.set_param<std::string>("error", std::string(e.what()));
  }
}

//! Run the FrameReceiverController
//!
//! This method runs the FrameReceiverController reactor event loop. Configuration
//! of the controller and associated objects is performed by calls to the
//! configure() method, either prior to calling run(), or in response to
//! appropriate messages being received on the appropriate IPC channel.
//!
void FrameReceiverController::run(void)
{
  LOG4CXX_TRACE(logger_, "FrameReceiverController::run()");
  terminate_controller_ = false;

  // Pre-charge all frame buffers onto the RX thread queue ready for use
  //precharge_buffers();

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Main thread entering reactor loop");

  // Run the reactor event loop
  reactor_.run();

  if (rx_thread_)
  {
    // Call cleanup on the RxThread
    rx_thread_->stop();

    // Destroy the RX thread
    rx_thread_.reset();
  }

  // Destroy the frame decoder
  if (frame_decoder_) {
    frame_decoder_.reset();
  }

  // Clean up IPC channels
  cleanup_ipc_channels();

}

//! Stop the FrameReceiverController
//
//! This method stops the controller by telling the reactor to stop.
//!
void FrameReceiverController::stop(void)
{
  LOG4CXX_TRACE(logger_, "FrameReceiverController::stop()");
  terminate_controller_ = true;
  reactor_.stop();
}


void FrameReceiverController::configure_ipc_channels(OdinData::IpcMessage& config_msg)
{

  bool force_reconfig = config_msg.get_param<bool>(CONFIG_FORCE_RECONFIG, false);

  // If a control endpoint is specified, bind the control channel
  if (config_msg.has_param(CONFIG_CTRL_ENDPOINT))
  {
    std::string endpoint = config_msg.get_param<std::string>(CONFIG_CTRL_ENDPOINT);
    if (force_reconfig || (endpoint != config_.ctrl_channel_endpoint_))
    {
      config_.ctrl_channel_endpoint_ = endpoint;
      this->setup_control_channel(endpoint);
    }
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
  reactor_.register_channel(frame_release_channel_,
            boost::bind(&FrameReceiverController::handle_frame_release_channel, this));

}

//! Clean up FrameReceiverController IPC channels
//!
//! This method cleans up controller IPC channels, removing them from the reactor and closing
//! the channels as appropriate.
//!
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
  std::string lib_dir; //(BUILD_DIR);
  //lib_dir += "/lib/";

  if (config_msg.has_param(CONFIG_DECODER_PATH))
  {
    lib_dir = config_msg.get_param<std::string>(CONFIG_DECODER_PATH);
  }
  else {
    lib_dir = config_.decoder_path_;
  }

  // Check if the last character is '/', if not append it
  if (*lib_dir.rbegin() != '/')
  {
    lib_dir += "/";
  }

  // If the incoming configuration specifies a frame decoder type, attempt to resolve and load
  // it from the appropriate library
  if (config_msg.has_param(CONFIG_DECODER_TYPE))
  {

    std::string decoder_type = config_msg.get_param<std::string>(CONFIG_DECODER_TYPE);
    if (decoder_type != Defaults::default_decoder_type)
    {
      std::string lib_name = "lib" + decoder_type + "FrameDecoder" + SHARED_LIBRARY_SUFFIX;
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

//! Configure the frame buffer manager
//!
//! This method configures the shared frame buffer manager used by the FrameReceiver to
//! store incoming frame data and hand over to downstream processing. The manager can only
//! be configured if a frame decoder has been loaded and can be interrogated for the
//! appropriate buffer information.
//!
//! \parmam[in] config_msg - IpcMessage containing configuration parameters
//!
void FrameReceiverController::configure_buffer_manager(OdinData::IpcMessage& config_msg)
{

  if (frame_decoder_)
  {
    if (config_msg.has_param(CONFIG_SHARED_BUFFER_NAME) &&
        config_msg.has_param(CONFIG_MAX_BUFFER_MEM))
    {

      std::string shared_buffer_name = config_msg.get_param<std::string>(CONFIG_SHARED_BUFFER_NAME);
      std::size_t max_buffer_mem = config_msg.get_param<std::size_t>(CONFIG_MAX_BUFFER_MEM);

      // Create a shared buffer manager
      buffer_manager_.reset(new SharedBufferManager(shared_buffer_name, max_buffer_mem,
                                                    frame_decoder_->get_frame_buffer_size(), false));

      LOG4CXX_DEBUG_LEVEL(1, logger_, "Configured frame buffer manager of total size " <<
          max_buffer_mem << " with " << buffer_manager_->get_num_buffers() << " buffers");

      // Register buffer manager with the frame decoder
      frame_decoder_->register_buffer_manager(buffer_manager_);

      // Notify downstream processes of current buffer configuration
      this->notify_buffer_config(true);

    }
  }
  else
  {
    LOG4CXX_INFO(logger_, "Shared frame buffer manager not configured as no frame decoder configured");
  }
}

//! Configure the receiver thread
//!
//! This method configures and launches the appropriate type of receiver thread based on
//! the configuration information specified in an IpcMessage.
//!
//! \parmam[in] config_msg - IpcMessage containing configuration parameters
//!
void FrameReceiverController::configure_rx_thread(OdinData::IpcMessage& config_msg)
{
  if (frame_decoder_ && buffer_manager_)
  {

    if (config_msg.has_param(CONFIG_RX_TYPE))
    {
      std::string rx_type_str = config_msg.get_param<std::string>(CONFIG_RX_TYPE);
      Defaults::RxType rx_type = FrameReceiverConfig::map_rx_name_to_type(rx_type_str);

      // Create the RX thread object
      switch(rx_type)
      {
        case Defaults::RxTypeUDP:
          rx_thread_.reset(new FrameReceiverUDPRxThread( config_, buffer_manager_, frame_decoder_));
          break;

        case Defaults::RxTypeZMQ:
          rx_thread_.reset(new FrameReceiverZMQRxThread( config_, buffer_manager_, frame_decoder_));
          break;

        default:
          throw FrameReceiverException("Cannot create RX thread - RX type not recognised");
      }
      rx_thread_->start();
    }
  }
  else
  {
    LOG4CXX_INFO(logger_, "RX thread not configured as frame decoder and/or buffer manager configured");
  }
}

void FrameReceiverController::precharge_buffers(void)
{
  if (buffer_manager_ && rx_thread_)
  {

    for (int buf = 0; buf < buffer_manager_->get_num_buffers(); buf++)
    {
      IpcMessage buf_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameRelease);
      buf_msg.set_param<int>("buffer_id", buf);
      rx_channel_.send(buf_msg.encode());
    }
  }
  else
  {
    LOG4CXX_INFO(logger_, "Buffer precharge not done as no buffer manager and/or RX thread configured");
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
  IpcMessage::MsgVal ctrl_reply_val = IpcMessage::MsgValIllegal;

  bool request_ok = true;
  std::ostringstream error_ss;

  // Parse and handle the message
  try {

    IpcMessage ctrl_req(ctrl_req_encoded.c_str(), false);
    IpcMessage::MsgType req_type = ctrl_req.get_msg_type();
    IpcMessage::MsgVal req_val = ctrl_req.get_msg_val();

    switch (req_type)
    {
      case IpcMessage::MsgTypeCmd:

        ctrl_reply.set_msg_val(req_val);

        switch (req_val)
        {
          case IpcMessage::MsgValCmdConfigure:
            LOG4CXX_DEBUG_LEVEL(3, logger_, "Got control channel configure request from client " << client_identity);
            this->configure(ctrl_req, ctrl_reply);
            break;

          default:
            request_ok = false;
            error_ss << "Illegal command request value: " << req_val;
            break;
        }
        break;

      default:
        request_ok = false;
        error_ss << "Illegal request type: " << req_type;
        break;
    }
  }
  catch (IpcMessageException& e)
  {
    request_ok = false;
    error_ss << e.what();
  }

  if (!request_ok) {
    LOG4CXX_ERROR(logger_, "Error handling control channel request from client "
                  << client_identity << ": " << error_ss.str());
    ctrl_reply.set_msg_type(IpcMessage::MsgTypeNack);
    ctrl_reply.set_param<std::string>("error", error_ss.str());
  }

  ctrl_channel_.send(ctrl_reply.encode(), 0, client_identity);

}

void FrameReceiverController::handle_rx_channel(void)
{
  std::string rx_reply_encoded = rx_channel_.recv();
  try {

    // LOG4CXX_DEBUG_LEVEL(1, logger_, "Got reply from RX thread : " << rx_reply_encoded);

    IpcMessage rx_reply(rx_reply_encoded.c_str());

    if ((rx_reply.get_msg_type() == IpcMessage::MsgTypeNotify) &&
        (rx_reply.get_msg_val() == IpcMessage::MsgValNotifyFrameReady))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame ready notification from RX thread"
          " for frame " << rx_reply.get_param<int>("frame", -1) <<
                        " in buffer " << rx_reply.get_param<int>("buffer_id", -1));
      frame_ready_channel_.send(rx_reply_encoded);

      frames_received_++;
    }
    else if ((rx_reply.get_msg_type() == IpcMessage::MsgTypeCmd) &&
        (rx_reply.get_msg_val() == IpcMessage::MsgValCmdBufferPrechargeRequest))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got buffer precharge request from RX thread");
      this->precharge_buffers();
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
