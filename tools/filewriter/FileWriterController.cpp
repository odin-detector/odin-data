/*
 * FileWriterController.cpp
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#include <FileWriterController.h>

#include <stdio.h>

namespace filewriter
{
  const std::string FileWriterController::CONFIG_SHUTDOWN          = "shutdown";

  const std::string FileWriterController::CONFIG_FR_SHARED_MEMORY  = "fr_shared_mem";
  const std::string FileWriterController::CONFIG_FR_RELEASE        = "fr_release_cnxn";
  const std::string FileWriterController::CONFIG_FR_READY          = "fr_ready_cnxn";
  const std::string FileWriterController::CONFIG_FR_SETUP          = "fr_setup";

  const std::string FileWriterController::CONFIG_CTRL_ENDPOINT     = "ctrl_endpoint";

  const std::string FileWriterController::CONFIG_LOAD_PLUGIN       = "load_plugin";
  const std::string FileWriterController::CONFIG_CONNECT_PLUGIN    = "connect_plugin";
  const std::string FileWriterController::CONFIG_DISCONNECT_PLUGIN = "disconnect_plugin";
  const std::string FileWriterController::CONFIG_PLUGIN_NAME       = "plugin_name";
  const std::string FileWriterController::CONFIG_PLUGIN_INDEX      = "plugin_index";
  const std::string FileWriterController::CONFIG_PLUGIN_CONNECT_TO = "plugin_connect_to";
  const std::string FileWriterController::CONFIG_PLUGIN_DISCONNECT_FROM = "plugin_disconnect_from";
  const std::string FileWriterController::CONFIG_PLUGIN_LIBRARY    = "plugin_library";

  FileWriterController::FileWriterController() :
    logger_(log4cxx::Logger::getLogger("FileWriterController")),
    runThread_(true),
    threadRunning_(false),
    threadInitError_(false),
    ctrlThread_(boost::bind(&FileWriterController::runIpcService, this)),
    ctrlChannel_(ZMQ_PAIR)
  {
    LOG4CXX_DEBUG(logger_, "Constructing FileWriterController");

    // Load in the default hdf5 writer plugin
    boost::shared_ptr<FileWriterPlugin> hdf5 = boost::shared_ptr<FileWriterPlugin>(new FileWriter());
    hdf5->setName("hdf");
    plugins_["hdf"] = hdf5;
    // Start the hdf5 worker thread
    hdf5->start();

    // Wait for the thread service to initialise and be running properly, so that
    // this constructor only returns once the object is fully initialised (RAII).
    // Monitor the thread error flag and throw an exception if initialisation fails
    while (!threadRunning_)
    {
        if (threadInitError_) {
            ctrlThread_.join();
            throw std::runtime_error(threadInitMsg_);
            break;
        }
    }

  }

  FileWriterController::~FileWriterController()
  {
    // TODO Auto-generated destructor stub
  }

  void FileWriterController::handleCtrlChannel()
  {
    // Receive a message from the main thread channel
    std::string ctrlMsgEncoded = ctrlChannel_.recv();

    LOG4CXX_DEBUG(logger_, "Control thread called with message: " << ctrlMsgEncoded);

    // Parse and handle the message
    try {
      FrameReceiver::IpcMessage ctrlMsg(ctrlMsgEncoded.c_str());

      //if ((rxMsg.get_msg_type() == FrameReceiver::IpcMessage::MsgTypeNotify) &&
      //    (rxMsg.get_msg_val()  == FrameReceiver::IpcMessage::MsgValNotifyFrameReady)){
      //} else {
      //  LOG4CXX_ERROR(logger_, "RX thread got unexpected message: " << rxMsgEncoded);
      //}
    }
    catch (FrameReceiver::IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
    }
  }

  void FileWriterController::configure(boost::shared_ptr<JSONMessage> config)
  {
    LOG4CXX_DEBUG(logger_, "Configuration submitted: " << config->toString());

    // Check if we are being asked to shutdown
    if (config->HasMember(FileWriterController::CONFIG_SHUTDOWN)){
      exitCondition_.notify_all();
    }

    if (config->HasMember(FileWriterController::CONFIG_CTRL_ENDPOINT)){
      std::string endpoint = (*config)[FileWriterController::CONFIG_CTRL_ENDPOINT].GetString();
      this->setupControlInterface(endpoint);
    }

    // Check if we are being passed the shared memory configuration
    if (config->HasMember(FileWriterController::CONFIG_FR_SETUP)){
      JSONMessage frConfig((*config)[FileWriterController::CONFIG_FR_SETUP]);
      if (frConfig.HasMember(FileWriterController::CONFIG_FR_SHARED_MEMORY) &&
          frConfig.HasMember(FileWriterController::CONFIG_FR_RELEASE) &&
          frConfig.HasMember(FileWriterController::CONFIG_FR_READY)){
        std::string shMemName = frConfig[FileWriterController::CONFIG_FR_SHARED_MEMORY].GetString();
        std::string pubString = frConfig[FileWriterController::CONFIG_FR_RELEASE].GetString();
        std::string subString = frConfig[FileWriterController::CONFIG_FR_READY].GetString();
        this->setupFrameReceiverInterface(shMemName, pubString, subString);
      }
    }

    // Check if we are being asked to load a plugin
    if (config->HasMember(FileWriterController::CONFIG_LOAD_PLUGIN)){
      JSONMessage pluginConfig((*config)[FileWriterController::CONFIG_LOAD_PLUGIN]);
      if (pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_NAME) &&
          pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_INDEX) &&
          pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_LIBRARY)){
        std::string index = pluginConfig[FileWriterController::CONFIG_PLUGIN_INDEX].GetString();
        std::string name = pluginConfig[FileWriterController::CONFIG_PLUGIN_NAME].GetString();
        std::string library = pluginConfig[FileWriterController::CONFIG_PLUGIN_LIBRARY].GetString();
        this->loadPlugin(index, name, library);
      }
    }

    // Check if we are being asked to connect a plugin
    if (config->HasMember(FileWriterController::CONFIG_CONNECT_PLUGIN)){
      JSONMessage pluginConfig((*config)[FileWriterController::CONFIG_CONNECT_PLUGIN]);
      if (pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_CONNECT_TO) &&
          pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_INDEX)){
        std::string index = pluginConfig[FileWriterController::CONFIG_PLUGIN_INDEX].GetString();
        std::string cnxn = pluginConfig[FileWriterController::CONFIG_PLUGIN_CONNECT_TO].GetString();
        this->connectPlugin(index, cnxn);
      }
    }

    // Check if we are being asked to disconnect a plugin
    if (config->HasMember(FileWriterController::CONFIG_DISCONNECT_PLUGIN)){
      JSONMessage pluginConfig((*config)[FileWriterController::CONFIG_DISCONNECT_PLUGIN]);
      if (pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_DISCONNECT_FROM) &&
          pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_INDEX)){
        std::string index = pluginConfig[FileWriterController::CONFIG_PLUGIN_INDEX].GetString();
        std::string cnxn = pluginConfig[FileWriterController::CONFIG_PLUGIN_DISCONNECT_FROM].GetString();
        this->disconnectPlugin(index, cnxn);
      }
    }

    // Loop over plugins, checking for configuration messages
    std::map<std::string, boost::shared_ptr<FileWriterPlugin> >::iterator iter;
    for (iter = plugins_.begin(); iter != plugins_.end(); ++iter){
      if (config->HasMember(iter->first)){
        boost::shared_ptr<JSONMessage> subConfig = boost::shared_ptr<JSONMessage>(new JSONMessage((*config)[iter->first]));
        iter->second->configure(subConfig);
      }
    }
  }

  void FileWriterController::loadPlugin(const std::string& index, const std::string& name, const std::string& library)
  {
    // Verify a plugin of the same name doesn't already exist
    if (plugins_.count(index) == 0){
      // Dynamically class load the plugin
      // Add the plugin to the map, indexed by the name
      boost::shared_ptr<FileWriterPlugin> plugin = ClassLoader<FileWriterPlugin>::load_class(name, library);
      plugin->setName(index);
      plugins_[index] = plugin;
      // Start the plugin worker thread
      plugin->start();
    } else {
      LOG4CXX_ERROR(logger_, "Cannot load plugin with index = " << index << ", already loaded");
    }
  }

  void FileWriterController::connectPlugin(const std::string& index, const std::string& connectTo)
  {
    // Check that the plugin is loaded
    if (plugins_.count(index) > 0){
      // Check for the shared memory connection
      if (connectTo == "frame_receiver"){
        sharedMemController_->registerCallback(index, plugins_[index]);
      } else {
        if (plugins_.count(connectTo) > 0){
          plugins_[connectTo]->registerCallback(index, plugins_[index]);
        }
      }
    } else {
      LOG4CXX_ERROR(logger_, "Cannot connet plugin with index = " << index << ", plugin isn't loaded");
    }
  }

  void FileWriterController::disconnectPlugin(const std::string& index, const std::string& disconnectFrom)
  {
    // Check that the plugin is loaded
    if (plugins_.count(index) > 0){
      // Check for the shared memory connection
      if (disconnectFrom == "frame_receiver"){
        sharedMemController_->removeCallback(index);
      } else {
        if (plugins_.count(disconnectFrom) > 0){
          plugins_[disconnectFrom]->removeCallback(index);
        }
      }
    } else {
      LOG4CXX_ERROR(logger_, "Cannot disconnet plugin with index = " << index << ", plugin isn't loaded");
    }
  }

  void FileWriterController::waitForShutdown()
  {
    boost::unique_lock<boost::mutex> lock(exitMutex_);
    exitCondition_.wait(lock);
  }

  void FileWriterController::setupFrameReceiverInterface(const std::string& sharedMemName,
                                                         const std::string& frPublisherString,
                                                         const std::string& frSubscriberString)
  {
    LOG4CXX_DEBUG(logger_, "Shared Memory Config: Name=" << sharedMemName <<
                  " Publisher=" << frPublisherString << " Subscriber=" << frSubscriberString);

    // Release current shared memory parser if one exists
    if (sharedMemParser_){
      sharedMemParser_.reset();
    }
    // Create the new shared memory parser
    sharedMemParser_ = boost::shared_ptr<SharedMemoryParser>(new SharedMemoryParser(sharedMemName));

    // Release the current zmq frame release publisher if one exists
//    if (frameReleasePublisher_){
//      frameReleasePublisher_.reset();
//    }
    // Now create the new zmq publisher
//    frameReleasePublisher_ = boost::shared_ptr<JSONPublisher>(new JSONPublisher(frPublisherString));
//    frameReleasePublisher_->connect();

    // Release the current shared memory controller if one exists
    if (sharedMemController_){
      sharedMemController_.reset();
    }
    // Create the new shared memory controller and give it the parser and publisher
    sharedMemController_ = boost::shared_ptr<SharedMemoryController>(new SharedMemoryController(reactor_, frSubscriberString, frPublisherString));
    sharedMemController_->setSharedMemoryParser(sharedMemParser_);
//    sharedMemController_->setFrameReleasePublisher(frameReleasePublisher_);

    // Release the current zmq frame ready subscriber if one exists
    //if (frameReadySubscriber_){
    //  frameReadySubscriber_.reset();
    //}
    // Now create the frame ready subscriber and register the shared memory controller
    //frameReadySubscriber_ = boost::shared_ptr<JSONSubscriber>(new JSONSubscriber(frSubscriberString));
    //frameReadySubscriber_->registerCallback(sharedMemController_);
    //frameReadySubscriber_->subscribe();

    // Register the default hdf5 writer plugin with the shared memory controller
    sharedMemController_->registerCallback("hdf", plugins_["hdf"]);
  }

  void FileWriterController::setupControlInterface(const std::string& ctrlEndpointString)
  {
    try {
      LOG4CXX_DEBUG(logger_, "Connecting control channel to endpoint: " << ctrlEndpointString);
      ctrlChannel_.bind(ctrlEndpointString.c_str());
    }
    catch (zmq::error_t& e) {
      //std::stringstream ss;
      //ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
      // TODO: What to do here, I think throw it up
      throw std::runtime_error(e.what());
    }

    // Add the control channel to the reactor
    reactor_->register_channel(ctrlChannel_, boost::bind(&FileWriterController::handleCtrlChannel, this));
  }

  void FileWriterController::runIpcService(void)
  {
    LOG4CXX_DEBUG(logger_, "Running IPC thread service");

    // Create the reactor
    reactor_ = boost::shared_ptr<FrameReceiver::IpcReactor>(new FrameReceiver::IpcReactor());

    // Add the tick timer to the reactor
    int tick_timer_id = reactor_->register_timer(1000, 0, boost::bind(&FileWriterController::tickTimer, this));

    // Add the buffer monitor timer to the reactor
    //int buffer_monitor_timer_id = reactor_.register_timer(3000, 0, boost::bind(&FrameReceiverRxThread::buffer_monitor_timer, this));

    // Register the frame release callback with the decoder
    //frame_decoder_->register_frame_ready_callback(boost::bind(&FrameReceiverRxThread::frame_ready, this, _1, _2));

    // Set thread state to running, allows constructor to return
    threadRunning_ = true;

    // Run the reactor event loop
    reactor_->run();

    // Cleanup - remove channels, sockets and timers from the reactor and close the receive socket
    LOG4CXX_DEBUG(logger_, "Terminating IPC thread service");
  }

  void FileWriterController::tickTimer(void)
  {
    LOG4CXX_DEBUG(logger_, "IPC thread tick timer fired");
    if (!runThread_)
    {
      LOG4CXX_DEBUG(logger_, "IPC thread terminate detected in timer");
      reactor_->stop();
    }
  }

} /* namespace filewriter */
