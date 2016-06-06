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
  const std::string FileWriterController::CONFIG_LOAD_PLUGIN       = "load_plugin";
  const std::string FileWriterController::CONFIG_CONNECT_PLUGIN    = "connect_plugin";
  const std::string FileWriterController::CONFIG_DISCONNECT_PLUGIN = "disconnect_plugin";
  const std::string FileWriterController::CONFIG_PLUGIN_NAME       = "plugin_name";
  const std::string FileWriterController::CONFIG_PLUGIN_CONNECTION = "plugin_connection";
  const std::string FileWriterController::CONFIG_PLUGIN_LIBRARY     = "plugin_library";

  FileWriterController::FileWriterController ()
  {
    // TODO Auto-generated constructor stub
    // Create shared memory parser
    sharedMemParser_ = boost::shared_ptr<SharedMemoryParser>(new SharedMemoryParser("FrameReceiverBuffer"));

    // create the zmq publisher
    frameReleasePublisher_ = boost::shared_ptr<JSONPublisher>(new JSONPublisher("tcp://127.0.0.1:5002"));
    frameReleasePublisher_->connect();

    sharedMemController_ = boost::shared_ptr<SharedMemoryController>(new SharedMemoryController());
    sharedMemController_->setSharedMemoryParser(sharedMemParser_);
    sharedMemController_->setFrameReleasePublisher(frameReleasePublisher_);

    // create the zmq subscriber
    frameReadySubscriber_ = boost::shared_ptr<JSONSubscriber>(new JSONSubscriber("tcp://127.0.0.1:5001"));
    frameReadySubscriber_->registerCallback(sharedMemController_);
    frameReadySubscriber_->subscribe();

    // Load in the default hdf5 writer plugin
    boost::shared_ptr<FileWriterPlugin> hdf5 = boost::shared_ptr<FileWriterPlugin>(new FileWriter());
    plugins_["hdf"] = hdf5;

    // Register the default hdf5 writer plugin with the shared memory controller
    sharedMemController_->registerCallback("hdf", plugins_["hdf"]);
  }

  FileWriterController::~FileWriterController ()
  {
    // TODO Auto-generated destructor stub
  }

  void FileWriterController::loadPlugin(const std::string& name, const std::string& library)
  {
    // Verify a plugin of the same name doesn't already exist
    // Dynamically class load the plugin
    // Add the plugin to the map, indexed by the name
    boost::shared_ptr<FileWriterPlugin> plugin = ClassLoader<FileWriterPlugin>::load_class(name, library);
  }

  void FileWriterController::callback(boost::shared_ptr<JSONMessage> msg)
  {
    printf("HERE!!! %s\n", msg->toString().c_str());
    // Check if we are being asked to load a plugin
    if (msg->HasMember(FileWriterController::CONFIG_LOAD_PLUGIN)){
      JSONMessage pluginConfig((*msg)[FileWriterController::CONFIG_LOAD_PLUGIN]);
      if (pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_NAME) &&
          pluginConfig.HasMember(FileWriterController::CONFIG_PLUGIN_LIBRARY)){
        std::string name = pluginConfig[FileWriterController::CONFIG_PLUGIN_NAME].GetString();
        std::string library = pluginConfig[FileWriterController::CONFIG_PLUGIN_LIBRARY].GetString();
        this->loadPlugin(name, library);
      }
    }
    // Loop over plugins, checking for configuration messages
    std::map<std::string, boost::shared_ptr<FileWriterPlugin> >::iterator iter;
    for (iter = plugins_.begin(); iter != plugins_.end(); ++iter){
      if (msg->HasMember(iter->first)){
        boost::shared_ptr<JSONMessage> subConfig = boost::shared_ptr<JSONMessage>(new JSONMessage((*msg)[iter->first]));
        iter->second->configure(subConfig);
      }
    }

    //if (msg->HasMember("hdf")){
    //  boost::shared_ptr<JSONMessage> hdf_msg = boost::shared_ptr<JSONMessage>(new JSONMessage((*msg)["hdf"]));
    //  plugins_["hdf"]->configure(hdf_msg);
    //}
  }

} /* namespace filewriter */
