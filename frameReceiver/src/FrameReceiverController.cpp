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
    need_ipc_reconfig_(false),
    need_decoder_reconfig_(false),
    need_buffer_manager_reconfig_(false),
    need_rx_thread_reconfig_(false),
    rx_channel_(ZMQ_ROUTER),
    ctrl_channel_(ZMQ_ROUTER),
    frame_ready_channel_(ZMQ_PUB),
    frame_release_channel_(ZMQ_SUB),
    frames_received_(0),
    frames_released_(0),
    rx_thread_identity_(RX_THREAD_ID)
{
  LOG4CXX_TRACE(logger_, "FrameRecevierController constructor");

}

//! Destructor for the FrameReceiverController.
//!
FrameReceiverController::~FrameReceiverController ()
{

  // Delete the RX thread object by resetting the scoped pointer, allowing the IPC channel
  // to be closed cleanly
  rx_thread_.reset();

}

//! Configure the FrameReceiverController.
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

  // Determine if a forced reconfiguration of all parts of the system is requested
  // and set up the individual reconfiguration flags appropriately. These can then
  // be modified at each configuration step to handle interdependencies.

  bool force_reconfig = config_msg.get_param<bool>(CONFIG_FORCE_RECONFIG, false);

  need_ipc_reconfig_ = force_reconfig;
  need_decoder_reconfig_ = force_reconfig;
  need_buffer_manager_reconfig_ = force_reconfig;
  need_rx_thread_reconfig_ = force_reconfig;

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

    // Construct the acknowledgement reply, indicating in the parameters which elements
    // have been reconfigured
    config_reply.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
    config_reply.set_param<bool>("ipc_reconfigured", need_ipc_reconfig_);
    config_reply.set_param<bool>("decoder_reconfigured", need_decoder_reconfig_);
    config_reply.set_param<bool>("buffer_manager_reconfigured", need_buffer_manager_reconfig_);
    config_reply.set_param<bool>("rx_thread_reconfigured", need_rx_thread_reconfig_);

  }
  catch (FrameReceiverException& e) {
    LOG4CXX_ERROR(logger_, "Configuration error: " << e.what());
    config_reply.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
    config_reply.set_param<std::string>("error", std::string(e.what()));
  }
}

//! Run the FrameReceiverController.
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

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Main thread entering reactor loop");

#ifdef FR_CONTROLLER_TICK_TIMER
  int tick_timer_id = reactor_.register_timer(
      deferred_action_delay_ms, 0, boost::bind(&FrameReceiverController::tick_timer, this));
#endif

  // Run the reactor event loop
  reactor_.run();

#ifdef FR_CONTROLLER_TICK_TIMER
  reactor_.remove_timer(tick_timer_id);
#endif

  // Stop the RX thread
  this->stop_rx_thread();

  // Destroy the frame decoder
  if (frame_decoder_) {
    frame_decoder_.reset();
  }

  // Clean up IPC channels
  cleanup_ipc_channels();

}

//! Stop the FrameReceiverController.
//
//! This method stops the controller by telling the reactor to stop. Execution of the
//! stop can be deferred to allow the process to respond to the shutdown request cleanly.
//!
//! \param[in] deferred - optional boolean flag indicating deferred execution requested
//!
void FrameReceiverController::stop(const bool deferred)
{
  if (deferred)
  {
    reactor_.register_timer(deferred_action_delay_ms, 1,
      boost::bind(&FrameReceiverController::stop, this, false)
    );
  }
  else 
  {
    LOG4CXX_TRACE(logger_, "FrameReceiverController::stop()");
    terminate_controller_ = true;
    reactor_.stop();
    }
}

//! Configure the necessary IPC channels.
//!
//! This method conditionally configures the IPC channels used by the controller
//! to communicate with control clients, the receiver thread and a downstream
//! frame processing application. Each of the channels is conditionally configured
//! if necessary or when the endpoint specified in the supplied configuration message
//! has changed.
//!
//! \param[in] config_msg - IPCMessage containing configuration parameters
//!
void FrameReceiverController::configure_ipc_channels(OdinData::IpcMessage& config_msg)
{

  // If a new control endpoint is specified, bind the control channel
  std::string ctrl_endpoint = config_msg.get_param<std::string>(
      CONFIG_CTRL_ENDPOINT, config_.ctrl_channel_endpoint_);
  if (need_ipc_reconfig_ || (ctrl_endpoint != config_.ctrl_channel_endpoint_))
  {
    this->unbind_channel(&ctrl_channel_, config_.ctrl_channel_endpoint_, true);
    config_.ctrl_channel_endpoint_ = ctrl_endpoint;
    this->setup_control_channel(ctrl_endpoint);
  }

  // If a new endpoint is specified, bind the RX thread channel
  std::string rx_endpoint = config_msg.get_param<std::string>(
      CONFIG_RX_ENDPOINT, config_.rx_channel_endpoint_);
  if (need_ipc_reconfig_ || (rx_endpoint != config_.rx_channel_endpoint_))
  {
    this->unbind_channel(&rx_channel_, config_.rx_channel_endpoint_, false);
    config_.rx_channel_endpoint_ = rx_endpoint;
    this->setup_rx_channel(rx_endpoint);

    // The RX thread will have to be reconfigured if this endpoint changes
    need_rx_thread_reconfig_ = true;
  }

  // If the endpoint is specified, bind the frame ready notification channel
  std::string frame_ready_endpoint = config_msg.get_param<std::string>(
      CONFIG_FRAME_READY_ENDPOINT, config_.frame_ready_endpoint_);
  if (need_ipc_reconfig_ || (frame_ready_endpoint != config_.frame_ready_endpoint_))
  {
    this->unbind_channel(&frame_ready_channel_, config_.frame_ready_endpoint_, false);
    config_.frame_ready_endpoint_ = frame_ready_endpoint;
    this->setup_frame_ready_channel(frame_ready_endpoint);
  }

  // If the endpoint is specified, bind the frame release notification channel
  std::string frame_release_endpoint = config_msg.get_param<std::string>(
      CONFIG_FRAME_RELEASE_ENDPOINT, config_.frame_release_endpoint_);
  if (need_ipc_reconfig_ || (frame_release_endpoint != config_.frame_release_endpoint_))
  {
    this->unbind_channel(&frame_release_channel_, config_.frame_release_endpoint_, false);
    config_.frame_release_endpoint_ = frame_release_endpoint;
    this->setup_frame_release_channel(frame_release_endpoint);
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

  try
  {
    ctrl_channel_.bind(endpoint.c_str());
    config_.ctrl_channel_endpoint_ = endpoint;
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding control channel endpoint " << endpoint << " failed: " << e.what();
    throw FrameReceiverException(sstr.str());
  }

  // Add channel to the reactor
  reactor_.register_channel(ctrl_channel_,
    boost::bind(&FrameReceiverController::handle_ctrl_channel, this)
  );

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
  reactor_.register_channel(rx_channel_,
    boost::bind(&FrameReceiverController::handle_rx_channel, this)
  );

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
  LOG4CXX_DEBUG_LEVEL(1, logger_,
                      "Binding frame ready notification channel to endpoint: " << endpoint);

  try {
    frame_ready_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding frame ready notification channel endpoint " << endpoint << " failed: "
        << e.what();
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
  LOG4CXX_DEBUG_LEVEL(1, logger_,
                      "Binding frame release notification channel to endpoint: " << endpoint);

  try {
    frame_release_channel_.bind(endpoint.c_str());
  }
  catch (zmq::error_t& e) {
    std::stringstream sstr;
    sstr << "Binding frame release notification channel endpoint " << endpoint << " failed: "
        << e.what();
    throw FrameReceiverException(sstr.str());
  }

  // Set default subscription on frame release channel
  frame_release_channel_.subscribe("");

  // Add channel to the reactor
  reactor_.register_channel(frame_release_channel_,
            boost::bind(&FrameReceiverController::handle_frame_release_channel, this));

}

//! Unbind an IpcChannel from and endpoint.
//!
//! This method unbinds the specified IPCChannel from an endpoint. This can be done in
//
void FrameReceiverController::unbind_channel(OdinData::IpcChannel* channel,
    std::string& endpoint, const bool deferred)
{
  if (channel->has_bound_endpoint(endpoint))
  {
    if (deferred)
    {
      reactor_.register_timer(deferred_action_delay_ms, 1,
          boost::bind(&FrameReceiverController::unbind_channel, this, channel, endpoint, false)
      );
    }
    else
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Unbinding channel endpoint " << endpoint);
      channel->unbind(endpoint);
    }
  }
  else
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Not unbinding channel as not bound to endpoint " << endpoint);
  }
}

//! Clean up FrameReceiverController IPC channels.
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

//! Configure the frame decoder.
//!
//! This method configures a frame decoder by loading the resolving the appropriate
//! library and class based on the input configuration.
//!
//! \param[in] config_msg - IPC message containing configuration parameters
//!
void FrameReceiverController::configure_frame_decoder(OdinData::IpcMessage& config_msg)
{

  // Resolve the decoder path if specified in the config message
  std::string decoder_path = config_msg.get_param<std::string>(
      CONFIG_DECODER_PATH, config_.decoder_path_);
  if (decoder_path != config_.decoder_path_)
  {
    config_.decoder_path_ = decoder_path;
    need_decoder_reconfig_ = true;
  }

  // Check if the last character is '/', if not append it
  if (*decoder_path.rbegin() != '/')
  {
    decoder_path += "/";
  }

  // Resolve the decoder type if specified in the config message
  std::string decoder_type = config_msg.get_param<std::string>(
      CONFIG_DECODER_TYPE, config_.decoder_type_);
  if (decoder_type != config_.decoder_type_) {
    config_.decoder_type_ = decoder_type;
    need_decoder_reconfig_ = true;
  }

  // Resolve, load and initialise the decoder class if necessary
  if (need_decoder_reconfig_)
  {
    if (decoder_type != Defaults::default_decoder_type)
    {
      std::string lib_name = "lib" + decoder_type + "FrameDecoder" + SHARED_LIBRARY_SUFFIX;
      std::string cls_name = decoder_type + "FrameDecoder";
      LOG4CXX_INFO(logger_, "Loading decoder plugin " << cls_name << " from "
          << decoder_path << lib_name);

      try {

        // The RX thread must be stopped and deleted first so that it releases its reference to the
        // current frame decoder (and shared buffer manager), allowing the current decoder instance
        // to be destroyed before reconfiguration.
        this->stop_rx_thread();

        frame_decoder_.reset();
        frame_decoder_ = OdinData::ClassLoader<FrameDecoder>::load_class(
            cls_name, decoder_path + lib_name);
        if (!frame_decoder_)
        {
          throw FrameReceiverException(
              "Cannot configure frame decoder: plugin type not recognised");
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

      // Extract any decoder parameters from the configuration message and construct an 
      // IpcMessage to pass to the decoder initialisation
      boost::scoped_ptr<IpcMessage> decoder_config(new IpcMessage());
      if (config_msg.has_param(CONFIG_DECODER_CONFIG))
      {
        decoder_config.reset(
          new IpcMessage(config_msg.get_param<const rapidjson::Value&>(CONFIG_DECODER_CONFIG))
        );
      }
      
      LOG4CXX_DEBUG_LEVEL(3, logger_, 
        "Built decoder configuration message: " << decoder_config->encode()
      );

      // Initialise the decoder object
      frame_decoder_->init(logger_, config_.enable_packet_logging_, config_.frame_timeout_ms_);

      // The buffer manager and RX thread will need reconfiguration if the decoder has been loaded
      // and initialised.
      need_buffer_manager_reconfig_ = true;
      need_rx_thread_reconfig_ = true;

    }
    else
    {
      LOG4CXX_INFO(logger_, "No frame decoder loaded: type not specified");
    }
  }
}

//! Configure the frame buffer manager.
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

  std::string shared_buffer_name = config_msg.get_param<std::string>(
      CONFIG_SHARED_BUFFER_NAME, config_.shared_buffer_name_);
  if (shared_buffer_name != config_.shared_buffer_name_)
  {
    config_.shared_buffer_name_ = shared_buffer_name;
    need_buffer_manager_reconfig_ = true;
  }

  std::size_t max_buffer_mem = config_msg.get_param<std::size_t>(
      CONFIG_MAX_BUFFER_MEM, config_.max_buffer_mem_);
  if (max_buffer_mem != config_.max_buffer_mem_)
  {
    config_.max_buffer_mem_ = max_buffer_mem;
    need_buffer_manager_reconfig_ = true;
  }

  if (need_buffer_manager_reconfig_)
  {
    if (frame_decoder_)
    {

      // The RX thread must be stopped and deleted first so that it releases its reference to the
      // current frame decoder (and shared buffer manager), allowing the current decoder instance to
      // be destroyed before reconfiguration.
      this->stop_rx_thread();

      // Instruct the frame decoder to drop any buffers currently in the empty queue or mapped for
      // receiving frames
      frame_decoder_->drop_all_buffers();

      // Create a new shared buffer manager
      buffer_manager_.reset(new SharedBufferManager(
          shared_buffer_name, max_buffer_mem,frame_decoder_->get_frame_buffer_size(), false)
      );

      LOG4CXX_DEBUG_LEVEL(1, logger_, "Configured frame buffer manager of total size " <<
          max_buffer_mem << " with " << buffer_manager_->get_num_buffers() << " buffers");

      // Register buffer manager with the frame decoder
      frame_decoder_->register_buffer_manager(buffer_manager_);

      // Notify downstream processes of current buffer configuration
      this->notify_buffer_config(true);

      // The RX thread will need reconfiguration if the buffer manager has been recreated
      need_rx_thread_reconfig_ = true;

    }
    else
    {
      LOG4CXX_INFO(logger_,
          "Shared frame buffer manager not configured as no frame decoder configured");
    }
  }
}

//! Configure the receiver thread.
//!
//! This method configures and launches the appropriate type of receiver thread based on
//! the configuration information specified in an IpcMessage.
//!
//! \parmam[in] config_msg - IpcMessage containing configuration parameters
//!
void FrameReceiverController::configure_rx_thread(OdinData::IpcMessage& config_msg)
{


  std::string rx_type_str = config_msg.get_param<std::string>(
      CONFIG_RX_TYPE, FrameReceiverConfig::map_rx_type_to_name(config_.rx_type_));
  Defaults::RxType rx_type = FrameReceiverConfig::map_rx_name_to_type(rx_type_str);

  if (rx_type != config_.rx_type_)
  {
    config_.rx_type_ = rx_type;
    need_rx_thread_reconfig_ = true;
  }

  if (need_rx_thread_reconfig_)
  {
    if (frame_decoder_ && buffer_manager_)
    {

      this->stop_rx_thread();

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
    else
    {
      LOG4CXX_INFO(logger_,
          "RX thread not configured as frame decoder and/or buffer manager configured");
    }
  }
}

//! Stop the receiver thread.
//!
//! This method stops the receiver thread cleanly and deletes the object instance by resetting
//! the scoped pointer.
//!
void FrameReceiverController::stop_rx_thread(void)
{
  if (rx_thread_)
  {
    // Signal to the RX thread to stop operation
    rx_thread_->stop();

    // Reset the scoped pointer to the RX thread
    rx_thread_.reset();
  }
}

//! Handle control channel messages.
//!
//! This method is the handler registered with the reactor to handle messages received
//! on the client control channel. Since this channel is implemented as ROUTER-DEALER,
//! the client identity of the incoming message is tracked and applied to the reply.
//!
void FrameReceiverController::handle_ctrl_channel(void)
{
  // Receive a message from the control channel, retrieving the client identity
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
            LOG4CXX_DEBUG_LEVEL(3, logger_,
                "Got control channel configure request from client " << client_identity);
            this->configure(ctrl_req, ctrl_reply);
            break;
          
          case IpcMessage::MsgValCmdShutdown:
              LOG4CXX_DEBUG_LEVEL(3, logger_,
                "Got shutdown command request from client " << client_identity);
              this->stop(true);
              ctrl_reply.set_msg_type(IpcMessage::MsgTypeAck);
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

  // Reply to the client on the control channel
  ctrl_channel_.send(ctrl_reply.encode(), 0, client_identity);

}

//! Handle receiver thread channel messages.
//!
//! This method is the handler registered with the reactor to handle messages received
//! on the receiver thread channel. Since this channel is implemented as ROUTER-DEALER,
//! the client identity of the incoming message is tracked and applied to the reply.
//! Frame ready notifications from the RX thread are passed onto the frame ready channel
//! to notify downstream processing applications that a new frame buffer is ready.
//!
void FrameReceiverController::handle_rx_channel(void)
{

  // Receive a message from the RX thread channel, retrieving the client identity
  std::string msg_indentity;
  std::string rx_msg_encoded = rx_channel_.recv(&msg_indentity);

  // Decode the messsage and handle appropriately
  try {

    IpcMessage rx_msg(rx_msg_encoded.c_str());
    IpcMessage::MsgType msg_type = rx_msg.get_msg_type();
    IpcMessage::MsgVal msg_val = rx_msg.get_msg_val();

    if ((msg_type == IpcMessage::MsgTypeNotify) &&
        (msg_val == IpcMessage::MsgValNotifyFrameReady))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame ready notification from RX thread"
          " for frame " << rx_msg.get_param<int>("frame", -1) <<
                        " in buffer " << rx_msg.get_param<int>("buffer_id", -1));
      frame_ready_channel_.send(rx_msg_encoded);

      frames_received_++;
    }
    else if ((msg_type == IpcMessage::MsgTypeNotify) &&
             (msg_val == IpcMessage::MsgValNotifyIdentity))
    {
      LOG4CXX_DEBUG_LEVEL(1, logger_,
          "Got identity announcement from RX thread: " << msg_indentity);
      rx_thread_identity_ = msg_indentity;
      IpcMessage rx_reply(IpcMessage::MsgTypeAck, IpcMessage::MsgValNotifyIdentity);
      rx_channel_.send(rx_reply.encode(), 0, rx_thread_identity_);
    }
    else if ((msg_type == IpcMessage::MsgTypeCmd) &&
             (msg_val == IpcMessage::MsgValCmdBufferPrechargeRequest))
    {
      LOG4CXX_DEBUG_LEVEL(2, logger_, "Got buffer precharge request from RX thread");
      this->precharge_buffers();
    }
    else
    {
      LOG4CXX_ERROR(logger_, "Got unexpected message from RX thread: " << rx_msg_encoded);
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding RX thread message: " << e.what());
  }
}

//! Handle frame release channel messages.
//!
//! This method is the handler registered with the reactor to handle messages received
//! on the frame release channel. Released frames are passed on to the RX thread so the
//! associated buffer can be queued for re-use in subsequent frame reception.
//!
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
      rx_channel_.send(frame_release_encoded, 0, rx_thread_identity_);

      frames_released_++;

      if (config_.frame_count_ && (frames_released_ >= config_.frame_count_))
      {
        LOG4CXX_INFO(logger_,
            "Specified number of frames (" << config_.frame_count_
            << ") received and released, terminating"
        );
        this->stop();
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
      LOG4CXX_ERROR(logger_,
          "Got unexpected message on frame release channel: " << frame_release_encoded);
    }
  }
  catch (IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_,
        "Error decoding message on frame release channel: " << e.what());
  }
}

//! Precharge empty frame buffers for use by the receiver thread.
//!
//! This method precharges all the buffers available in the shared buffer manager onto the
//! empty buffer queue in the receiver thread. This allows the receiver thread to obtain a pool
//! of empty buffers at startup, and is done by sending frame release notificaitons for all buffers
//! over the RX thread channel.
//!
void FrameReceiverController::precharge_buffers(void)
{

  // Only pre-charge buffers if a buffer manager and RX thread are configured
  if (buffer_manager_ && rx_thread_)
  {

    // Loop over all buffers in the buffer manager and send a frame release notification for each
    for (int buf = 0; buf < buffer_manager_->get_num_buffers(); buf++)
    {
      IpcMessage buf_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameRelease);
      buf_msg.set_param<int>("buffer_id", buf);
      rx_channel_.send(buf_msg.encode(), 0, rx_thread_identity_);
    }
  }
  else
  {
    LOG4CXX_INFO(logger_,
       "Buffer precharge not done as no buffer manager and/or RX thread configured");
  }
}

//! Notify downstream processing applications of the current shared buffer configuration.
//!
//! This method notifies processing applications subscribed to the frame ready channel of the
//! current shared buffer notification. This is done at startup and any time the shared buffer
//! configuration changes. Execution of the notification can be deferred by setting the optional
//! argument, allowing time for IPC channels to be set up and downstream applications to be
//! responsive to notifications.
//!
//! \param[in] deferred - optional boolean flag indicating deferred execution requested
//!
void FrameReceiverController::notify_buffer_config(const bool deferred)
{
  // Notify downstream applications listening on the frame ready channel of the current shared
  // buffer configuration.

  if (deferred)
  {
    reactor_.register_timer(deferred_action_delay_ms, 1,
      boost::bind(&FrameReceiverController::notify_buffer_config, this, false)
    );
  }
  else
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_,
        "Notifying downstream processes of shared buffer configuration");

    IpcMessage config_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyBufferConfig);
    config_msg.set_param("shared_buffer_name", config_.shared_buffer_name_);

    frame_ready_channel_.send(config_msg.encode());
  }
}

#ifdef FR_CONTROLLER_TICK_TIMER
//! Diagnostic tick timer for the frame controller.
//!
//! This ticker timer can be used for debugging the controller event loop. When compiled in it will
//! will be registered to fire periodically in the reactor
//!
void FrameReceiverController::tick_timer(void)
{
  LOG4CXX_DEBUG_LEVEL(4, logger_, "Controller tick timer fired");
}
#endif
