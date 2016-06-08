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

  const std::string FileWriterController::CONFIG_LOAD_PLUGIN       = "load_plugin";
  const std::string FileWriterController::CONFIG_CONNECT_PLUGIN    = "connect_plugin";
  const std::string FileWriterController::CONFIG_DISCONNECT_PLUGIN = "disconnect_plugin";
  const std::string FileWriterController::CONFIG_PLUGIN_NAME       = "plugin_name";
  const std::string FileWriterController::CONFIG_PLUGIN_INDEX      = "plugin_index";
  const std::string FileWriterController::CONFIG_PLUGIN_CONNECT_TO = "plugin_connect_to";
  const std::string FileWriterController::CONFIG_PLUGIN_DISCONNECT_FROM = "plugin_disconnect_from";
  const std::string FileWriterController::CONFIG_PLUGIN_LIBRARY    = "plugin_library";

  FileWriterController::FileWriterController() :
    logger_(log4cxx::Logger::getLogger("FileWriterController"))
  {
    LOG4CXX_DEBUG(logger_, "Constructing FileWriterController");

    // Load in the default hdf5 writer plugin
    boost::shared_ptr<FileWriterPlugin> hdf5 = boost::shared_ptr<FileWriterPlugin>(new FileWriter());
    hdf5->setName("hdf");
    plugins_["hdf"] = hdf5;
    // Start the hdf5 worker thread
    hdf5->start();
  }

  FileWriterController::~FileWriterController()
  {
    // TODO Auto-generated destructor stub
  }

  void FileWriterController::configure(boost::shared_ptr<JSONMessage> config)
  {
    LOG4CXX_DEBUG(logger_, "Configuration submitted: " << config->toString());

    // Check if we are being asked to shutdown
    if (config->HasMember(FileWriterController::CONFIG_SHUTDOWN)){
      exitCondition_.notify_all();
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

  void FileWriterController::callback(boost::shared_ptr<JSONMessage> msg)
  {
    // Pass straight to configure
    this->configure(msg);
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
    if (frameReleasePublisher_){
      frameReleasePublisher_.reset();
    }
    // Now create the new zmq publisher
    frameReleasePublisher_ = boost::shared_ptr<JSONPublisher>(new JSONPublisher(frPublisherString));
    frameReleasePublisher_->connect();

    // Release the current shared memory controller if one exists
    if (sharedMemController_){
      sharedMemController_.reset();
    }
    // Create the new shared memory controller and give it the parser and publisher
    sharedMemController_ = boost::shared_ptr<SharedMemoryController>(new SharedMemoryController());
    sharedMemController_->setSharedMemoryParser(sharedMemParser_);
    sharedMemController_->setFrameReleasePublisher(frameReleasePublisher_);

    // Release the current zmq frame ready subscriber if one exists
    if (frameReadySubscriber_){
      frameReadySubscriber_.reset();
    }
    // Now create the frame ready subscriber and register the shared memory controller
    frameReadySubscriber_ = boost::shared_ptr<JSONSubscriber>(new JSONSubscriber(frSubscriberString));
    frameReadySubscriber_->registerCallback(sharedMemController_);
    frameReadySubscriber_->subscribe();

    // Register the default hdf5 writer plugin with the shared memory controller
    sharedMemController_->registerCallback("hdf", plugins_["hdf"]);
  }

} /* namespace filewriter */
