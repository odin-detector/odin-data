/*
 * FrameProcessorPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include "logging.h"
#include "FrameProcessorPlugin.h"
#include "DebugLevelLogger.h"
#include "gettime.h"

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
std::string FrameProcessorPlugin::get_name() const
{
  // Return our name
  return name_;
}

/** Set the error state.
 *
 * Sets an error for this plugin
 *
 * \param[in] msg - std::string error message.
 */
void FrameProcessorPlugin::set_error(const std::string& msg)
{
  // Take lock to access error_messages_
  boost::lock_guard<boost::mutex> lock(mutex_);

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
    LOG4CXX_ERROR(logger_, msg);
  }
}

/** Set the warning state.
 *
 * Sets an warning for this plugin
 *
 * \param[in] msg - std::string warning message.
 */
void FrameProcessorPlugin::set_warning(const std::string& msg)
{
  // Take lock to access warning_messages_
  boost::lock_guard<boost::mutex> lock(mutex_);

  // Loop over warning messages, if this is a new message then add it
  std::vector<std::string>::iterator iter;
  bool found_warning = false;
  for (iter = warning_messages_.begin(); iter != warning_messages_.end(); ++iter){
    if (msg == *iter){
      found_warning = true;
    }
  }
  if (!found_warning){
    warning_messages_.push_back(msg);
    LOG4CXX_WARN(logger_, msg);
  }
}

/** Clear error and warning messages.
 */
void FrameProcessorPlugin::clear_errors()
{
  // Take lock to access error_messages_
  boost::lock_guard<boost::mutex> lock(mutex_);
  error_messages_.clear();
  warning_messages_.clear();
}

/** Reset any statistics.
 *
 * Any counters in the plugin should be reset by this method
 */
bool FrameProcessorPlugin::reset_statistics()
{
  // Default method does nothing
  return true;
}

/** Return the current error message.
 *
 */
std::vector<std::string> FrameProcessorPlugin::get_errors()
{
  // Take lock to access error_messages_
  boost::lock_guard<boost::mutex> lock(mutex_);
  return error_messages_;
}

/** Return the current warning message.
 *
 */
std::vector<std::string> FrameProcessorPlugin::get_warnings()
{
  // Take lock to access warning_messages_
  boost::lock_guard<boost::mutex> lock(mutex_);
  return warning_messages_;
}

/** Configure the plugin.
 *
 * In this abstract class the configure method doesn't perform any
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
 * In this abstract class the request method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[out] reply - Response IpcMessage with current configuration.
 */
void FrameProcessorPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  // Default method simply does nothing
}

/** Request the plugin's configuration Metadata.
 * \param[out] reply - Response IpcMessage with current config metadata.
 */
void FrameProcessorPlugin::requestConfigurationMetadata(OdinData::IpcMessage& reply) const
{
  auto end = config_metadata_bucket_.end();
  for(auto itr = config_metadata_bucket_.begin(); itr != end; ++itr) {
    add_metadata(reply, *itr);
  }
}

/** Request the plugin's status Metadata.
 * \param[out] reply - Response IpcMessage with current status metadata.
 */
void FrameProcessorPlugin::requestStatusMetadata(OdinData::IpcMessage& reply) const
{
  auto end = status_metadata_bucket_.end();
  for(auto itr = status_metadata_bucket_.begin(); itr != end; ++itr) {
    add_metadata(reply, *itr);
  }
}

/** Execute a command within the plugin.
 *
 * In this abstract class the command method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[in] command - String containing the command to execute.
 * \param[out] reply - Response IpcMessage.
 */
void FrameProcessorPlugin::execute(const std::string& command, OdinData::IpcMessage& reply)
{
  // A command has been submitted and this plugin has no execute implementation defined,
  // throw a runtime error to report this.
  std::stringstream ss;
  ss << "Submitted command not supported: " << command;
  LOG4CXX_ERROR(logger_, ss.str());
  reply.set_nack(ss.str());
}

/** Request the plugin's supported commands.
 *
 * In this abstract class the request method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \return - Vector containing supported command strings.
 */
std::vector<std::string> FrameProcessorPlugin::requestCommands()
{
  // Default returns an empty vector.
  std::vector<std::string> reply;
  return reply;
}

/**
 * Collate status information for the plugin.
 *
 * The status is added to the status IpcMessage object.
 * In this abstract class the status method doesn't perform any
 * actions, this should be overridden by subclasses.
 *
 * \param[out] status - Reference to an IpcMessage value to store the status.
 */
void FrameProcessorPlugin::status(OdinData::IpcMessage& status)
{
  // Default method simply does nothing
}

/**
 * Collate performance statistics for the plugin.
 *
 * The performance metrics are added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the performance stats.
 */
void FrameProcessorPlugin::add_performance_stats(OdinData::IpcMessage& status)
{
  status.set_param(get_name() + "/timing/last_process", process_duration_.last_);
  status.set_param(get_name() + "/timing/max_process", process_duration_.max_);
  status.set_param(get_name() + "/timing/mean_process", process_duration_.mean_);
}

/**
 * Reset performance statistics for the plugin.
 *
 * The performance metrics are reset to zero.
 */
void FrameProcessorPlugin::reset_performance_stats()
{
  process_duration_.reset();
}

/**
 * Collate version information for the plugin.
 *
 * The version information is added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the version.
 */
void FrameProcessorPlugin::version(OdinData::IpcMessage& status)
{
  status.set_param("version/" + get_name() + "/major", get_version_major());
  status.set_param("version/" + get_name() + "/minor", get_version_minor());
  status.set_param("version/" + get_name() + "/patch", get_version_patch());
  status.set_param("version/" + get_name() + "/short", get_version_short());
  status.set_param("version/" + get_name() + "/full", get_version_long());
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
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Registering blocking callback " << name << " with " << name_);
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
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Registering non-blocking callback " << name << " with " << name_);
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
 * Remove all registered callbacks for this plugin.
 */
void FrameProcessorPlugin::remove_all_callbacks()
{
  // Loop over blocking callbacks, removing each one
  std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator bcbIter;
  for (bcbIter = blocking_callbacks_.begin(); bcbIter != blocking_callbacks_.end(); ++bcbIter) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Removing callback " << bcbIter->first << " from " << name_);
    bcbIter->second->confirmRemoval(name_);
  }
  // Loop over non-blocking callbacks, removing each one
  std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
  for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Removing callback " << cbIter->first << " from " << name_);
    cbIter->second->confirmRemoval(name_);
  }
  // Now empty the two callback stores
  callbacks_.clear();
  blocking_callbacks_.clear();
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
  // Calls process frame and times how long the process takes
  struct timespec start_time;
  struct timespec end_time;
  // Check if the frame is tagged as an end of acquisition
  if (frame->get_end_of_acquisition()){
    // This frame is tagged as EOA.  Call the cleanup method
    // and then automatically push the frame object
    this->process_end_of_acquisition();
    this->push(frame);
  } else {
    // This is a standard frame so process and record the time taken
    gettime(&start_time);
    this->process_frame(frame);
    gettime(&end_time);
    uint64_t ts = elapsed_us(start_time, end_time);
    // Update process_frame performance stats
    process_duration_.update(ts);
  }
}

void FrameProcessorPlugin::notify_end_of_acquisition()
{
  // Create an EndOfAcquisitionFrame object and push it through the processing chain
  boost::shared_ptr<EndOfAcquisitionFrame> eoa = boost::shared_ptr<EndOfAcquisitionFrame>(new EndOfAcquisitionFrame());
  this->push(eoa);
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
  if (!frame->get_end_of_acquisition() && !frame->is_valid()){
    throw std::runtime_error("FrameProcessorPlugin::push Invalid frame pushed onto plugin chain");
  }
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

/** Push the supplied frame to a specifically named registered callback.
 *
 * This method calls the named blocking callback directly or places the frame
 * pointer on the named worker queue (see IFrameCallback).
 *
 * \param[in] plugin_name - Name of the plugin to send the frame to.
 * \param[in] frame - Pointer to the frame.
 */
void FrameProcessorPlugin::push(const std::string& plugin_name, boost::shared_ptr<Frame> frame)
{
  if (!frame->get_end_of_acquisition() && !frame->is_valid()){
    throw std::runtime_error("FrameProcessorPlugin::push Invalid frame pushed onto plugin chain");
  }
  if (blocking_callbacks_.find(plugin_name) != blocking_callbacks_.end()){
    blocking_callbacks_[plugin_name]->callback(frame);
  }
  if (callbacks_.find(plugin_name) != callbacks_.end()){
    callbacks_[plugin_name]->getWorkQueue()->add(frame);
  }
}

/** Perform any end of acquisition cleanup.
 *
 * This default implementation does nothing.  Any plugins that want to perform
 * cleanup actions when an end of acquisition notification takes place can
 * override this method.
 */
void FrameProcessorPlugin::process_end_of_acquisition()
{

}

} /* namespace FrameProcessor */
