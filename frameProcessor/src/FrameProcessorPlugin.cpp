/*
 * FrameProcessorPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include "logging.h"
#include "FrameProcessorPlugin.h"

namespace FrameProcessor
{

/**
 * Constructor, initialises name_ and meta data channel.
 */
FrameProcessorPlugin::FrameProcessorPlugin() :
    name_("")
{
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  logger_ = log4cxx::Logger::getLogger("FP.FrameProcessorPlugin");
}

/**
 * Destructor
 */
FrameProcessorPlugin::~FrameProcessorPlugin()
{
  // TODO Auto-generated destructor stub
}

/**
 * Set the name of this plugin
 *
 * \param[in] name - The name.
 */
void FrameProcessorPlugin::set_name(const std::string& name)
{
  // Record our name
  name_ = name;
}

/**
 * Get the name of this plugin
 *
 * \return The name.
 */
std::string FrameProcessorPlugin::get_name()
{
  // Return our name
  return name_;
}

/** Set the error state.
 *
 * Sets the error level for this plugin
 *
 * \param[in] msg - std::string error message.
 * \param[in] level - int error level of the message.
 */
void FrameProcessorPlugin::set_error(const std::string& msg)
{
  // Loop over error messages, if this is a new message then add it
  std::vector<std::string>::iterator iter;
  bool found_error = false;
  for (iter = error_messages_.begin(); iter != error_messages_.end(); ++iter){
    if (msg == *iter){
      found_error = true;
    }
  }
  if (!found_error){
    error_messages_.push_back(msg);
  }
}

/** Clear any error state.
 *
 * Clears the error level and any message
 */
void FrameProcessorPlugin::clear_errors()
{
  error_messages_.clear();
}

/** Return the current error message.
 *
 */
std::vector<std::string> FrameProcessorPlugin::get_errors()
{
  return error_messages_;
}

    /** Configure the plugin.
 *
 * In this abstract class the configure method does perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FrameProcessorPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Default method simply does nothing
}

/** Request the plugin's current configuration.
 *
 * In this abstract class the request method does perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[out] reply - Response IpcMessage with current configuration.
 */
void FrameProcessorPlugin::requestConfiguration(OdinData::IpcMessage& reply)
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
void FrameProcessorPlugin::status(OdinData::IpcMessage& status)
{
  // Default method simply does nothing
}

/**
 * Registers another plugin for frame callbacks from this plugin.
 *
 * The callback interface (which will be another plugin) is stored in
 * our map, indexed by name. If the callback already exists within our
 * map then this is a no-op.
 *
 * \param[in] name - Index of the callback (plugin index).
 * \param[in] cb - Pointer to an IFrameCallback interface (plugin).
 * \param[in] blocking - Whether call should block.
 */
void FrameProcessorPlugin::register_callback(const std::string& name,
                                            boost::shared_ptr<IFrameCallback> cb, bool blocking)
{
  if (blocking) {
    if (callbacks_.count(name) != 0) {
      LOG4CXX_WARN(logger_, "Non-blocking callback " << name << " already registered with " << name_ <<
                            ". Must be removed before adding blocking callback");
    }
      // Check if we own the callback already
    else if (blocking_callbacks_.count(name) == 0) {
      LOG4CXX_DEBUG(logger_, "Registering blocking callback " << name << " with " << name_);
      // Record the callback pointer
      blocking_callbacks_[name] = cb;
      // Confirm registration
      cb->confirmRegistration(name_);
    }
  }
  else {
    if (blocking_callbacks_.count(name) != 0) {
      LOG4CXX_WARN(logger_, "Blocking callback " << name << " already registered with " << name_ <<
                            ". Must be removed before adding non-blocking callback");
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
void FrameProcessorPlugin::remove_callback(const std::string& name)
{
  boost::shared_ptr<IFrameCallback> cb;
  if (callbacks_.count(name) > 0) {
    // Get the pointer
    cb = callbacks_[name];
    // Remove the callback from the map
    callbacks_.erase(name);
    // Confirm removal
    cb->confirmRemoval(name_);
  }
  else if (blocking_callbacks_.count(name) > 0) {
    // Get the pointer
    cb = blocking_callbacks_[name];
    // Remove the callback from the map
    blocking_callbacks_.erase(name);
    // Confirm removal
    cb->confirmRemoval(name_);
  }
}

/**
 * We have been called back with a frame from a plugin that we registered
 * with. This method calls the processFrame pure virtual method that
 * must be overridden by any children of this abstract class.
 *
 * \param[in] frame - Pointer to the frame.
 */
void FrameProcessorPlugin::callback(boost::shared_ptr<Frame> frame)
{
  // Calls process frame
  this->process_frame(frame);
}

/** Push the supplied frame to any registered callbacks.
 *
 * This method calls any blocking callbacks directly and then loops over the
 * map of registered callbacks and places the frame pointer on their worker
 * queue (see IFrameCallback).
 *
 * \param[in] frame - Pointer to the frame.
 */
void FrameProcessorPlugin::push(boost::shared_ptr<Frame> frame)
{
  // Loop over blocking callbacks, calling each function and waiting for return
  std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator bcbIter;
  for (bcbIter = blocking_callbacks_.begin(); bcbIter != blocking_callbacks_.end(); ++bcbIter) {
    bcbIter->second->callback(frame);
  }
  // Loop over non-blocking callbacks, placing frame onto each queue
  std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
  for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter) {
    cbIter->second->getWorkQueue()->add(frame);
  }
}

} /* namespace FrameProcessor */
