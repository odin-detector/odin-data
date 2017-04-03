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
    logger_ = log4cxx::Logger::getLogger("FW.FileWriterPlugin");
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
  void FileWriterPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
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
  void FileWriterPlugin::status(OdinData::IpcMessage& status)
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
  void FileWriterPlugin::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking)
  {
    if (blocking) {
      if (callbacks_.count(name) != 0) {
        LOG4CXX_WARN(logger_, "Non-blocking callback " << name << " already registered with " << name_ << ". Must be removed before adding blocking callback");
      }
      // Check if we own the callback already
      else if (blockingCallbacks_.count(name) == 0) {
        LOG4CXX_DEBUG(logger_, "Registering blocking callback " << name << " with " << name_);
        // Record the callback pointer
        blockingCallbacks_[name] = cb;
        // Confirm registration
        cb->confirmRegistration(name_);
      }
    }
    else {
      if (blockingCallbacks_.count(name) != 0) {
        LOG4CXX_WARN(logger_, "Blocking callback " << name << " already registered with " << name_ << ". Must be removed before adding non-blocking callback");
      }
      // Check if we own the callback already
      else if (callbacks_.count(name) == 0) {
        LOG4CXX_DEBUG(logger_, "Registering non-blocking callback " << name << " with " << name_);
        // Record the callback pointer
        callbacks_[name] = cb;
        // Confirm registration
        cb->confirmRegistration(name_);
      }
    }
  }

  /**
   * Remove a plugin from our callback map.
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
    else if (blockingCallbacks_.count(name) > 0) {
      // Get the pointer
      cb = blockingCallbacks_[name];
      // Remove the callback from the map
      blockingCallbacks_.erase(name);
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
    // Loop over blocking callbacks, calling each function and waiting for return
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator bcbIter;
    for (bcbIter = blockingCallbacks_.begin(); bcbIter != blockingCallbacks_.end(); ++bcbIter) {
      bcbIter->second->callback(frame);
    }
    // Loop over non-blocking callbacks, placing frame onto each queue
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
    for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter) {
      cbIter->second->getWorkQueue()->add(frame);
    }
  }

} /* namespace filewriter */
