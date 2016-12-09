/*
 * FileWriterPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include <FileWriterPlugin.h>

namespace filewriter
{

  /**
   * Constructor, initialises name_.
   */
  FileWriterPlugin::FileWriterPlugin() :
      name_("")
  {
  }

  /**
   * Destructor
   */
  FileWriterPlugin::~FileWriterPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  /**
   * Set the name of this plugin
   *
   * \param[in] name - The name.
   */
  void FileWriterPlugin::setName(const std::string& name)
  {
    // Record our name
    name_ = name;
  }

  /**
   * Get the name of this plugin
   *
   * \return The name.
   */
  std::string FileWriterPlugin::getName()
  {
    // Return our name
    return name_;
  }

  /** Configure the plugin.
   *
   * In this abstract class the configure method does perform any
   * actions, this should be overridden by subclasses.
   *
   * \param[in] config - IpcMessage containing configuration data.
   * \param[out] reply - Response IpcMessage.
   */
  void FileWriterPlugin::configure(FrameReceiver::IpcMessage& config, FrameReceiver::IpcMessage& reply)
  {
    // Default method simply does nothing
  }

  /**
   * Collate status information for the plugin.
   *
   * The status is added to the status IpcMessage object.
   * In this abstract class the status method does perform any
   * actions, this should be overridden by subclasses.
   *
   * \param[out] status - Reference to an IpcMessage value to store the status.
   */
  void FileWriterPlugin::status(FrameReceiver::IpcMessage& status)
  {
    // Default method simply does nothing
  }

  /**
   * Registers another plugin for frame callbacks from this plugin.
   *
   * The callback interface (which will be another plugin) is stored in
   * our map, indexed by name.  If the callback already exists within our
   * map then this is a no-op.
   *
   * \param[in] name - Index of the callback (plugin index).
   * \param[in] cb - Pointer to an IFrameCallback interface (plugin).
   */
  void FileWriterPlugin::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb)
  {
    // Check if we own the callback already
    if (callbacks_.count(name) == 0){
      // Record the callback pointer
      callbacks_[name] = cb;
      // Confirm registration
      cb->confirmRegistration(name_);
    }
  }

  /**
   * Removes another plugin from our callback map.
   *
   * \param[in] name - Index of the callback (plugin index) to remove.
   */
  void FileWriterPlugin::removeCallback(const std::string& name)
  {
    boost::shared_ptr<IFrameCallback> cb;
    if (callbacks_.count(name) > 0){
      // Get the pointer
      cb = callbacks_[name];
      // Remove the callback from the map
      callbacks_.erase(name);
      // Confirm removal
      cb->confirmRemoval(name_);
    }
  }

  /**
   * We have been called back with a frame from a plugin that we registered
   * with.  This method calls the processFrame pure virtual method that
   * must be overridden by any children of this abstract class.
   *
   * \param[in] frame - Pointer to the frame.
   */
  void FileWriterPlugin::callback(boost::shared_ptr<Frame> frame)
  {
    // Calls process frame
    this->processFrame(frame);
  }

  /** Push the supplied frame to any registered callbacks.
   *
   * This method loops over the map of registered callbacks and places
   * the frame pointer on their worker queue (see IFrameCallback).
   *
   * \param[in] frame - Pointer to the frame.
   */
  void FileWriterPlugin::push(boost::shared_ptr<Frame> frame)
  {
    // Loop over callbacks, placing frame onto each queue
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
    for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
      cbIter->second->getWorkQueue()->add(frame);
    }
  }

} /* namespace filewriter */
