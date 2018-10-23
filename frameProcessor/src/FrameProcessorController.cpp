/*
 * FrameProcessorController.cpp
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#include <stdio.h>

#include "FrameProcessorController.h"
#include "DebugLevelLogger.h"

namespace FrameProcessor
{

const std::string FrameProcessorController::META_RX_INTERFACE        = "inproc://meta_rx";

const std::string FrameProcessorController::CONFIG_SHUTDOWN          = "shutdown";

const std::string FrameProcessorController::CONFIG_DEBUG             = "debug_level";

const std::string FrameProcessorController::CONFIG_FR_RELEASE        = "fr_release_cnxn";
const std::string FrameProcessorController::CONFIG_FR_READY          = "fr_ready_cnxn";
const std::string FrameProcessorController::CONFIG_FR_SETUP          = "fr_setup";

const std::string FrameProcessorController::CONFIG_CTRL_ENDPOINT     = "ctrl_endpoint";
const std::string FrameProcessorController::CONFIG_META_ENDPOINT     = "meta_endpoint";

const std::string FrameProcessorController::CONFIG_PLUGIN            = "plugin";
const std::string FrameProcessorController::CONFIG_PLUGIN_LOAD       = "load";
const std::string FrameProcessorController::CONFIG_PLUGIN_CONNECT    = "connect";
const std::string FrameProcessorController::CONFIG_PLUGIN_DISCONNECT = "disconnect";
const std::string FrameProcessorController::CONFIG_PLUGIN_NAME       = "name";
const std::string FrameProcessorController::CONFIG_PLUGIN_INDEX      = "index";
const std::string FrameProcessorController::CONFIG_PLUGIN_LIBRARY    = "library";
const std::string FrameProcessorController::CONFIG_PLUGIN_CONNECTION = "connection";

const int FrameProcessorController::META_TX_HWM = 10000;

/** Construct a new FrameProcessorController class.
 *
 * The constructor sets up logging used within the class, and starts the
 * IpcReactor thread.
 */
FrameProcessorController::FrameProcessorController() :
    logger_(log4cxx::Logger::getLogger("FP.FrameProcessorController")),
    runThread_(true),
    threadRunning_(false),
    threadInitError_(false),
    pluginShutdownSent(false),
    ctrlThread_(boost::bind(&FrameProcessorController::runIpcService, this)),
    ctrlChannelEndpoint_(""),
    ctrlChannel_(ZMQ_ROUTER),
    metaRxChannel_(ZMQ_PULL),
    metaTxChannelEndpoint_(""),
    metaTxChannel_(ZMQ_PUB),
    frReadyEndpoint_(""),
    frReleaseEndpoint_("")
{
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Constructing FrameProcessorController");

  totalFrames = 0;

  // Wait for the thread service to initialise and be running properly, so that
  // this constructor only returns once the object is fully initialised (RAII).
  // Monitor the thread error flag and throw an exception if initialisation fails
  while (!threadRunning_)
  {
    if (threadInitError_) {
      ctrlThread_.join();
      throw std::runtime_error(threadInitMsg_);
    }
  }

  // The meta interface should only be setup once confirmation of the thread startup
  // has been recevied. The reactor is created within the thread startup method.
  this->setupMetaRxInterface();
}

/**
 * Destructor.
 */
FrameProcessorController::~FrameProcessorController()
{
  // TODO Auto-generated destructor stub
}

/** Handle an incoming configuration message.
 *
 * This method is called by the IpcReactor when a configuration IpcMessage
 * has been received. The raw message is read and parsed into an IpcMessage
 * for further processing. The configure method is called, and once
 * configuration has completed a response IpcMessage is sent back on the
 * control channel.
 */
void FrameProcessorController::handleCtrlChannel()
{
  // Receive a message from the main thread channel
  std::string clientIdentity;
  std::string ctrlMsgEncoded = ctrlChannel_.recv(&clientIdentity);
  unsigned int msg_id = 0;
  
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread called with message: " << ctrlMsgEncoded);

  // Parse and handle the message
  try {
    OdinData::IpcMessage ctrlMsg(ctrlMsgEncoded.c_str());
    OdinData::IpcMessage replyMsg;  // Instantiate default IpmMessage
    replyMsg.set_msg_val(ctrlMsg.get_msg_val());
    msg_id = ctrlMsg.get_msg_id();
    replyMsg.set_msg_id(msg_id);

    if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
        (ctrlMsg.get_msg_val()  == OdinData::IpcMessage::MsgValCmdConfigure)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      this->configure(ctrlMsg, replyMsg);
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (configure): "
                             << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdRequestConfiguration)) {
        replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
        this->requestConfiguration(replyMsg);
        LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (request configuration): "
                               << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdStatus)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      this->provideStatus(replyMsg);
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (status): "
                             << replyMsg.encode());
    }
    else if ((ctrlMsg.get_msg_type() == OdinData::IpcMessage::MsgTypeCmd) &&
             (ctrlMsg.get_msg_val() == OdinData::IpcMessage::MsgValCmdResetStatistics)) {
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
      this->resetStatistics(replyMsg);
      LOG4CXX_DEBUG_LEVEL(3, logger_, "Control thread reply message (reset statistics): "
              << replyMsg.encode());
    }
    else {
      LOG4CXX_ERROR(logger_, "Control thread got unexpected message: " << ctrlMsgEncoded);
      replyMsg.set_param("error", "Invalid control message: " + ctrlMsgEncoded);
      replyMsg.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
    }
    ctrlChannel_.send(replyMsg.encode(), 0, clientIdentity);
  }
  catch (OdinData::IpcMessageException& e)
  {
    LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
  }
  catch (std::runtime_error& e)
  {
    LOG4CXX_ERROR(logger_, "Bad control message: " << e.what());
    OdinData::IpcMessage replyMsg(OdinData::IpcMessage::MsgTypeNack, OdinData::IpcMessage::MsgValCmdConfigure);
    replyMsg.set_param<std::string>("error", std::string(e.what()));
    replyMsg.set_msg_id(msg_id);
    ctrlChannel_.send(replyMsg.encode());
  }
}

void FrameProcessorController::handleMetaRxChannel()
{
  uintptr_t pValue;
  FrameProcessor::MetaMessage *rPtr;

  // Receive a message from the main thread channel
  metaRxChannel_.recv_raw(&pValue);
  // Message contains the pointer value, cast back into MetaMessage object
  rPtr = reinterpret_cast<FrameProcessor::MetaMessage *>(pValue);

  // Create the two part message ready to publish. First part contains
  // header information and second part contains the actual data
  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Value nameValue;
  nameValue.SetString(rPtr->getName().c_str(), rPtr->getName().length(), doc.GetAllocator());
  rapidjson::Value itemValue;
  itemValue.SetString(rPtr->getItem().c_str(), rPtr->getItem().length(), doc.GetAllocator());
  rapidjson::Value typeValue;
  typeValue.SetString(rPtr->getType().c_str(), rPtr->getType().length(), doc.GetAllocator());

  rapidjson::Value headerValue;
  rapidjson::Document headerDoc;
  // Attempt to parse the header
  if (headerDoc.Parse(rPtr->getHeader().c_str()).HasParseError()) {
    // Unable to parse the header, so copy it as a string
    headerValue.SetString(rPtr->getHeader().c_str(), rPtr->getHeader().length(), doc.GetAllocator());
  } else {
    // Copy the parsed document to the header value
    headerValue.CopyFrom(headerDoc, doc.GetAllocator());
  }

  // Create the information to the JSON doc
  doc.AddMember("plugin", nameValue, doc.GetAllocator());
  doc.AddMember("parameter", itemValue, doc.GetAllocator());
  doc.AddMember("type", typeValue, doc.GetAllocator());
  doc.AddMember("header", headerValue, doc.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);

  LOG4CXX_TRACE(logger_, "Meta RX thread called: " << buffer.GetString());

  metaTxChannel_.send(buffer.GetString(), ZMQ_SNDMORE);
  metaTxChannel_.send(rPtr->getSize(), rPtr->getDataPtr());

  // Delete the meta message
  delete rPtr;
}

/**
 * Count frames passed through plugin chain and trigger shutdown when expected datasets received.
 *
 * This is registered as a blocking callback from the end of the plugin chain.
 * It will check for shutdown conditions and then return once all plugins have been
 * notified to shutdown by the main thread.
 *
 * @param frame - Pointer to the frame
 */
void FrameProcessorController::callback(boost::shared_ptr<Frame> frame) {

  // If frame is a master frame, or all frames are included (no master frames), increment frame count
  if (masterFrame == "" || frame->get_dataset_name() == masterFrame) {
    totalFrames++;
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Frame " << totalFrames << " complete.");
  }

  if (totalFrames == datasetSize) {
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Dataset complete. Shutting down.");
    // Set exit condition so main thread can continue and shutdown
    exitCondition_.notify_all();
    // Wait until the main thread has sent stop commands to the plugins
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Exit condition set. Waiting for main thread to stop plugins.");
    while(!pluginShutdownSent);
    // Return so they can shutdown
  }
}

/** Provide status information to requesting clients.
 *
 * This is called in response to a status request from a connected client. The reply to the
 * request is populated with status information from the shared memory controller and all the
 * plugins currently loaded, and with any error messages currently stored.
 *
 * @param[in,out] reply - response IPC message to be populated with status parameters
 */
void FrameProcessorController::provideStatus(OdinData::IpcMessage& reply)
{
  // Error messages
  std::vector<std::string> error_messages;

  // Request status information from the shared memory controller
  if (sharedMemController_) {
    sharedMemController_->status(reply);
  }

  // Loop over plugins, list names and request status from each
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator iter;
  for (iter = plugins_.begin(); iter != plugins_.end(); ++iter) {
    reply.set_param("plugins/names[]", iter->first);
    iter->second->status(reply);
    // Read error level
    std::vector<std::string> plugin_errors = iter->second->get_errors();
    error_messages.insert(error_messages.end(), plugin_errors.begin(), plugin_errors.end());
  }
  std::vector<std::string>::iterator error_iter;
  for (error_iter = error_messages.begin(); error_iter != error_messages.end(); ++error_iter) {
    reply.set_param("error[]", *error_iter);
  }
}

/**
 * Set configuration options for the FrameProcessorController.
 *
 * Sets up the overall FileWriter application according to the
 * configuration IpcMessage objects that are received. The objects
 * are searched for:
 * CONFIG_SHUTDOWN - Shuts down the application
 * CONFIG_STATUS - Retrieves status for all plugins and replies
 * CONFIG_CTRL_ENDPOINT - Calls the method setupControlInterface
 * CONFIG_PLUGIN - Calls the method configurePlugin
 * CONFIG_FR_SETUP - Calls the method setupFrameReceiverInterface
 *
 * The method also searches for configuration objects that have the
 * same index as loaded plugins. If any of these are found the they
 * are passed down to the plugin for execution.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FrameProcessorController::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Configuration submitted: " << config.encode());

  // Check if we are being given the master frame specifier
  if (config.has_param("hdf/master")) {
    masterFrame = config.get_param<std::string>("hdf/master");
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Master frame specifier set to: " << masterFrame);
  }

  // Check if we have been asked to reset any errors
  if (config.has_param("clear_errors")) {
    std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator iter;
    for (iter = plugins_.begin(); iter != plugins_.end(); ++iter) {
      iter->second->clear_errors();
    }
  }

  // If single-shot and frames given then we are running for defined number and then shutting down
  if (config.has_param("single-shot") && config.get_param<bool>("single-shot") &&
      config.has_param("frames") && config.get_param<unsigned int>("frames") != 0) {
    datasetSize = config.get_param<unsigned int>("frames");
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Dataset size: " << datasetSize);
  }

  // Check for a debug level change
  if (config.has_param(FrameProcessorController::CONFIG_DEBUG)) {
    unsigned int debug_level = config.get_param<unsigned int>(FrameProcessorController::CONFIG_DEBUG);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug_level);
    set_debug_level(debug_level);
  }

  // Check if we are being asked to shutdown
  if (config.has_param(FrameProcessorController::CONFIG_SHUTDOWN)) {
    exitCondition_.notify_all();
  }

  if (config.has_param(FrameProcessorController::CONFIG_CTRL_ENDPOINT)) {
    std::string endpoint = config.get_param<std::string>(FrameProcessorController::CONFIG_CTRL_ENDPOINT);
    this->setupControlInterface(endpoint);
  }

  if (config.has_param(FrameProcessorController::CONFIG_META_ENDPOINT)) {
    std::string endpoint = config.get_param<std::string>(FrameProcessorController::CONFIG_META_ENDPOINT);
    this->setupMetaTxInterface(endpoint);
  }

  if (config.has_param(FrameProcessorController::CONFIG_PLUGIN)) {
    OdinData::IpcMessage pluginConfig(config.get_param<const rapidjson::Value&>(FrameProcessorController::CONFIG_PLUGIN));
    this->configurePlugin(pluginConfig, reply);
  }

  // Check if we are being passed the shared memory configuration
  if (config.has_param(FrameProcessorController::CONFIG_FR_SETUP)) {
    OdinData::IpcMessage frConfig(config.get_param<const rapidjson::Value&>(FrameProcessorController::CONFIG_FR_SETUP));
    if (frConfig.has_param(FrameProcessorController::CONFIG_FR_RELEASE) &&
        frConfig.has_param(FrameProcessorController::CONFIG_FR_READY)) {
      std::string pubString = frConfig.get_param<std::string>(FrameProcessorController::CONFIG_FR_RELEASE);
      std::string subString = frConfig.get_param<std::string>(FrameProcessorController::CONFIG_FR_READY);
      this->setupFrameReceiverInterface(pubString, subString);
    }
  }

  // Loop over plugins, checking for configuration messages
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator iter;
  for (iter = plugins_.begin(); iter != plugins_.end(); ++iter) {
    if (config.has_param(iter->first)) {
      OdinData::IpcMessage subConfig(config.get_param<const rapidjson::Value&>(iter->first));
      iter->second->configure(subConfig, reply);
    }
  }
}

/**
 * Request the current configuration of the FrameProcessorController.
 *
 * The method also searches through all loaded plugins.  Each plugin is
 * also sent a request for its configuration.
 *
 * \param[out] reply - Response IpcMessage with the current configuration.
 */
void FrameProcessorController::requestConfiguration(OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(3, logger_, "Request for configuration made");

  // Add local configuration parameter values to the reply
  reply.set_param(FrameProcessorController::CONFIG_CTRL_ENDPOINT, ctrlChannelEndpoint_);
  reply.set_param(FrameProcessorController::CONFIG_META_ENDPOINT, metaTxChannelEndpoint_);
  std::string fr_cnxn_str = FrameProcessorController::CONFIG_FR_SETUP + "/";
  reply.set_param(fr_cnxn_str + FrameProcessorController::CONFIG_FR_READY, frReadyEndpoint_);
  reply.set_param(fr_cnxn_str + FrameProcessorController::CONFIG_FR_RELEASE, frReleaseEndpoint_);

  // Loop over plugins and request current configuration from each
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator iter;
  for (iter = plugins_.begin(); iter != plugins_.end(); ++iter) {
    iter->second->requestConfiguration(reply);
  }
}

/**
 * Reset statistics on all of the loaded plugins.
 *
 * The method calls reset statistics on all of the loaded plugins.
 *
 * \param[out] reply - Response IpcMessage with the current status.
 */
void FrameProcessorController::resetStatistics(OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Reset statistics requested");
  bool reset_ok = true;

  // Loop over plugins and call reset statistics on each
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator iter;
  for (iter = plugins_.begin(); iter != plugins_.end(); ++iter) {
    reset_ok = iter->second->reset_statistics();
    // Check for failure
    if (!reset_ok){
      reply.set_msg_type(OdinData::IpcMessage::MsgTypeNack);
      std::stringstream sstr;
      sstr << "Failed to reset statistics on plugin " << iter->first;
      reply.set_param("error", sstr.str());
    }
  }
}

/**
 * Set configuration options for the plugins.
 *
 * Sets up the plugins loaded into the controller according to the
 * configuration IpcMessage objects that are received. The objects
 * are searched for:
 * CONFIG_PLUGIN_LIST - Replies with a list of loaded plugins
 * CONFIG_PLUGIN_LOAD - Uses NAME, INDEX and LIBRARY to load a plugin
 * into the controller.
 * CONFIG_PLUGIN_CONNECT - Uses CONNECTION and INDEX to connect one
 * plugin input to another plugin output.
 * CONFIG_PLUGIN_DISCONNECT - Uses CONNECTION and INDEX to disconnect
 * one plugin from another.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FrameProcessorController::configurePlugin(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check if we are being asked to load a plugin
  if (config.has_param(FrameProcessorController::CONFIG_PLUGIN_LOAD)) {
    OdinData::IpcMessage pluginConfig(config.get_param<const rapidjson::Value&>(FrameProcessorController::CONFIG_PLUGIN_LOAD));
    if (pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_NAME) &&
        pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_INDEX) &&
        pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_LIBRARY)) {
      std::string index = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_INDEX);
      std::string name = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_NAME);
      std::string library = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_LIBRARY);
      this->loadPlugin(index, name, library);
    }
  }

  // Check if we are being asked to connect a plugin
  if (config.has_param(FrameProcessorController::CONFIG_PLUGIN_CONNECT)) {
    OdinData::IpcMessage pluginConfig(config.get_param<const rapidjson::Value&>(FrameProcessorController::CONFIG_PLUGIN_CONNECT));
    if (pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_CONNECTION) &&
        pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_INDEX)) {
      std::string index = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_INDEX);
      std::string cnxn = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_CONNECTION);
      this->connectPlugin(index, cnxn);
    }
  }

  // Check if we are being asked to disconnect a plugin
  if (config.has_param(FrameProcessorController::CONFIG_PLUGIN_DISCONNECT)) {
    OdinData::IpcMessage pluginConfig(config.get_param<const rapidjson::Value&>(FrameProcessorController::CONFIG_PLUGIN_DISCONNECT));
    if (pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_CONNECTION) &&
        pluginConfig.has_param(FrameProcessorController::CONFIG_PLUGIN_INDEX)) {
      std::string index = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_INDEX);
      std::string cnxn = pluginConfig.get_param<std::string>(FrameProcessorController::CONFIG_PLUGIN_CONNECTION);
      this->disconnectPlugin(index, cnxn);
    }
  }
}

/** Load a new plugin.
 *
 * Attempts to load the specified library dynamically using the classloader.
 * If the index specified is already used then throws an error. Once the plugin
 * has been loaded it's processing thread is started. The same plugin type can
 * be loaded multiple times as long as each index is unique.
 *
 * \param[in] index - Unique index required for the plugin.
 * \param[in] name - Name of the plugin class.
 * \param[in] library - Full path of shared library file for the plugin.
 */
void FrameProcessorController::loadPlugin(const std::string& index, const std::string& name, const std::string& library)
{
  // Verify a plugin of the same name doesn't already exist
  if (plugins_.count(index) == 0) {
    // Dynamically class load the plugin
    // Add the plugin to the map, indexed by the name
    boost::shared_ptr<FrameProcessorPlugin> plugin = OdinData::ClassLoader<FrameProcessorPlugin>::load_class(name, library);
    if (plugin) {
      plugin->set_name(index);
      plugin->connect_meta_channel();
      plugins_[index] = plugin;

      // Register callback to FWC with FileWriter plugin
      if (name == "FileWriter") {
        plugin->register_callback("controller", this->shared_from_this(), true);
      }

      // Start the plugin worker thread
      plugin->start();
    } else {
      LOG4CXX_ERROR(logger_, "Could not load plugin with index [" << index <<
                                                                  "], name [" << name << "], check library");
      std::stringstream is;
      is << "Cannot load plugin with index [" << index << "], name [" << name << "], check library";
      throw std::runtime_error(is.str().c_str());
    }
  } else {
    LOG4CXX_INFO(logger_, "Plugin with index = " << index << ", already loaded");
  }
}

/** Connects two plugins together.
 *
 * When plugins have been connected they can pass frame objects between them.
 *
 * \param[in] index - Index of the plugin wanting to connect.
 * \param[in] connectTo - Index of the plugin to connect to.
 */
void FrameProcessorController::connectPlugin(const std::string& index, const std::string& connectTo)
{
  // Check that the plugin is loaded
  if (plugins_.count(index) > 0) {
    // Check for the shared memory connection
    if (connectTo == "frame_receiver") {
      if (sharedMemController_) {
        sharedMemController_->registerCallback(index, plugins_[index]);
      } else {
        LOG4CXX_ERROR(logger_, "Cannot connect " << index <<
                                                 " to frame_receiver, frame_receiver is not configured");
        std::stringstream is;
        is << "Cannot connect " << index << " to frame_receiver, frame_receiver is not configured";
        throw std::runtime_error(is.str().c_str());
      }
    } else {
      if (plugins_.count(connectTo) > 0) {
        plugins_[connectTo]->register_callback(index, plugins_[index]);
      }
    }
  } else {
    LOG4CXX_ERROR(logger_, "Cannot connect plugin with index = " << index << ", plugin isn't loaded");
    std::stringstream is;
    is << "Cannot connect plugin with index = " << index << ", plugin isn't loaded";
    throw std::runtime_error(is.str().c_str());
  }
}

/** Disconnect one plugin from another plugin.
 *
 * \param[in] index - Index of the plugin wanting to disconnect.
 * \param[in] disconnectFrom - Index of the plugin to disconnect from.
 */
void FrameProcessorController::disconnectPlugin(const std::string& index, const std::string& disconnectFrom)
{
  // Check that the plugin is loaded
  if (plugins_.count(index) > 0) {
    // Check for the shared memory connection
    if (disconnectFrom == "frame_receiver") {
      sharedMemController_->removeCallback(index);
    } else {
      if (plugins_.count(disconnectFrom) > 0) {
        plugins_[disconnectFrom]->remove_callback(index);
      }
    }
  } else {
    LOG4CXX_ERROR(logger_, "Cannot disconnect plugin with index = " << index << ", plugin isn't loaded");
    std::stringstream is;
    is << "Cannot disconnect plugin with index = " << index << ", plugin isn't loaded";
    throw std::runtime_error(is.str().c_str());
  }
}

void FrameProcessorController::run() {

  LOG4CXX_INFO(logger_, "Running frame processor");

  // Start worker thread (for IFrameCallback) to monitor frames passed through
  start();

  // Now wait for the shutdown command from either the control interface or the worker thread
  waitForShutdown();

  // Stop all plugin worker threads
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Stopping plugin worker threads");
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> >::iterator it;
  for (it = plugins_.begin(); it != plugins_.end(); it++) {
    it->second->stop();
  }
  // Worker thread callback will block caller until pluginShutdownSent is set
  pluginShutdownSent = true;
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Plugin shutdown sent. Removing plugins once stopped.");
  // Wait until the each plugin has stopped and erase it from our map
  for (it = plugins_.begin(); it != plugins_.end(); it++) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Removing " << it->first);
    while(it->second->isWorking());
    plugins_.erase(it);
  }

  // Stop worker thread (for IFrameCallback) and reactor
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Stopping FrameProcessorController worker thread and IPCReactor");
  stop();
  reactor_->stop();

  // Close control IPC channel
  closeControlInterface();
  // Close FrameReceiver interface IPC channels
  closeFrameReceiverInterface();

  // Destroy any allocated DataBlocks
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Tearing down DataBlockPool");
  DataBlockPool::tearDownClass();

  LOG4CXX_INFO(logger_, "Shutting Down.");
}

/**
 * Wait for the exit condition before returning.
 */
void FrameProcessorController::waitForShutdown()
{
  boost::unique_lock<boost::mutex> lock(exitMutex_);
  exitCondition_.wait(lock);
}

/** Set up the frame receiver interface.
 *
 * This method creates new SharedMemoryController and SharedMemoryParser objects,
 * which manage the receipt of frame ready notifications and construction of
 * Frame objects from shared memory.
 * Pointers to the two objects are kept by this class.
 *
 * \param[in] sharedMemName - Name of the shared memory block opened by the frame receiver.
 * \param[in] frPublisherString - Endpoint for sending frame release notifications.
 * \param[in] frSubscriberString - Endpoint for receiving frame ready notifications.
 */
void FrameProcessorController::setupFrameReceiverInterface(const std::string& frPublisherString,
                                                           const std::string& frSubscriberString)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Shared Memory Config: Publisher=" << frPublisherString << " Subscriber=" << frSubscriberString);

  // Only re-construct the shared memory object if either of the endpoints has been changed
  if (frPublisherString != frReleaseEndpoint_ || frSubscriberString != frReadyEndpoint_){
    try
    {
      // Release the current shared memory controller if one exists
      if (sharedMemController_) {
        sharedMemController_.reset();
      }
      // Create the new shared memory controller and give it the parser and publisher
      sharedMemController_ = boost::shared_ptr<SharedMemoryController>(
          new SharedMemoryController(reactor_, frSubscriberString, frPublisherString));
      frReadyEndpoint_ = frSubscriberString;
      frReleaseEndpoint_ = frPublisherString;

    } catch (const boost::interprocess::interprocess_exception& e)
    {
      LOG4CXX_ERROR(logger_, "Unable to access shared memory: " << e.what());
    }
  } else {
    LOG4CXX_ERROR(logger_, "*** Not updating shared memory, endpoints were not changed");
  }
}

/** Close the frame receiver interface.
 */
void FrameProcessorController::closeFrameReceiverInterface()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing FrameReceiver interface.");
  try
  {
    // Release the current shared memory controller if one exists
    if (sharedMemController_) {
      sharedMemController_.reset();
    }
  } catch (const boost::interprocess::interprocess_exception& e)
  {
    LOG4CXX_ERROR(logger_, "Error occurred when closing FrameReceiver interface: " << e.what());
  }

}

/** Set up the control interface.
 *
 * This method binds the control IpcChannel to the provided endpoint,
 * creating a socket for controlling applications to connect to. This
 * socket is used for sending configuration IpcMessages.
 *
 * \param[in] ctrlEndpointString - Name of the control endpoint.
 */
void FrameProcessorController::setupControlInterface(const std::string& ctrlEndpointString)
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting control channel to endpoint: " << ctrlEndpointString);
    ctrlChannel_.bind(ctrlEndpointString.c_str());
    ctrlChannelEndpoint_ = ctrlEndpointString;
  }
  catch (zmq::error_t& e) {
    //std::stringstream ss;
    //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
    // TODO: What to do here, I think throw it up
    throw std::runtime_error(e.what());
  }

  // Add the control channel to the reactor
  reactor_->register_channel(ctrlChannel_, boost::bind(&FrameProcessorController::handleCtrlChannel, this));
}

/** Close the control interface.
 */
void FrameProcessorController::closeControlInterface()
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing control endpoint socket.");
    ctrlThread_.join();
    reactor_->remove_channel(ctrlChannel_);
    ctrlChannel_.close();
  }
  catch (zmq::error_t& e) {
    // TODO: What to do here, I think throw it up
    throw std::runtime_error(e.what());
  }
}

void FrameProcessorController::setupMetaRxInterface()
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting meta RX channel to endpoint: " << META_RX_INTERFACE);
    metaRxChannel_.bind(META_RX_INTERFACE.c_str());
  }
  catch (zmq::error_t& e) {
    throw std::runtime_error(e.what());
  }

  // Add the control channel to the reactor
  reactor_->register_channel(metaRxChannel_, boost::bind(&FrameProcessorController::handleMetaRxChannel, this));
}

void FrameProcessorController::closeMetaRxInterface()
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing meta RX endpoint.");
    reactor_->remove_channel(metaRxChannel_);
    metaRxChannel_.close();
  }
  catch (zmq::error_t& e) {
    throw std::runtime_error(e.what());
  }
}

void FrameProcessorController::setupMetaTxInterface(const std::string& metaEndpointString)
{
  try {
    LOG4CXX_DEBUG(logger_, "Connecting meta TX channel to endpoint: " << metaEndpointString);
    int sndHwmSet = META_TX_HWM;
    metaTxChannel_.setsockopt(ZMQ_SNDHWM, &sndHwmSet, sizeof (sndHwmSet));
    metaTxChannel_.bind(metaEndpointString.c_str());
    metaTxChannelEndpoint_ = metaEndpointString;
  }
  catch (zmq::error_t& e) {
    throw std::runtime_error(e.what());
  }
}

void FrameProcessorController::closeMetaTxInterface()
{
  try {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing meta TX endpoint.");
    metaTxChannel_.close();
  }
  catch (zmq::error_t& e) {
    throw std::runtime_error(e.what());
  }
}

/** Start the Ipc service running.
 *
 * Sets up a tick timer and runs the Ipc reactor.
 * Currently the tick timer does not perform any processing.
 */
void FrameProcessorController::runIpcService(void)
{
  // Configure logging for this thread
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running IPC thread service");

  // Create the reactor
  reactor_ = boost::shared_ptr<OdinData::IpcReactor>(new OdinData::IpcReactor());

  // Add the tick timer to the reactor
  int tick_timer_id = reactor_->register_timer(1000, 0, boost::bind(&FrameProcessorController::tickTimer, this));

  // Set thread state to running, allows constructor to return
  threadRunning_ = true;

  // Run the reactor event loop
  reactor_->run();

  // Cleanup - remove channels, sockets and timers from the reactor and close the receive socket
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Terminating IPC thread service");
}

/** Tick timer task called by IpcReactor.
 *
 * This currently performs no processing.
 */
void FrameProcessorController::tickTimer(void)
{
  if (!runThread_)
  {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "IPC thread terminate detected in timer");
    reactor_->stop();
  }
}

} /* namespace FrameProcessor */
