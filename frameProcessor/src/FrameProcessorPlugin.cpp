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
  name_(""),
  last_process_time_(0),
  max_process_time_(0),
  average_process_time_(0.0)
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
 * Sets an error for this plugin
 *
 * \param[in] msg - std::string error message.
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
    LOG4CXX_ERROR(logger_, msg);
  }
}

/** Clear any error state.
 *
 * Clears any messages that have previously been set
 */
void FrameProcessorPlugin::clear_errors()
{
  error_messages_.clear();
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
 * Collate performance statistics for the plugin.
 *
 * The performance metrics are added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the performance stats.
 */
void FrameProcessorPlugin::add_performance_stats(OdinData::IpcMessage& status)
{
  status.set_param(get_name() + "/timing/last_process", last_process_time_);
  status.set_param(get_name() + "/timing/max_process", max_process_time_);
  status.set_param(get_name() + "/timing/mean_process", average_process_time_);
}

/**
 * Reset performance statistics for the plugin.
 *
 * The performance metrics are reset to zero.
 */
void FrameProcessorPlugin::reset_performance_stats()
{
  last_process_time_ = 0;
  max_process_time_ = 0;
  average_process_time_ = 0.0;
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
  gettime(&start_time);
  this->process_frame(frame);
  gettime(&end_time);
  uint64_t ts = elapsed_us(start_time, end_time);
  // Store the raw process time
  last_process_time_ = ts;
  // Store the maximum process time since the last reset
  if (ts > max_process_time_){
    max_process_time_ = ts;
  }
  // Store a simple exp average with alpha = 0.5
  average_process_time_ = (average_process_time_ * 0.5) + (double(ts) * 0.5);
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
  if (!frame->is_valid())
      throw std::runtime_error("Invalid frame");
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

/** Calculate and return an elapsed time in microseconds.
 * 
 * This method calculates and returns an elapsed time in microseconds based on the start and
 * end timespec structs passed as arguments.
 * 
 * \param[in] start - start time in timespec struct format
 * \param[in] end - end time in timespec struct format
 * \return elapsed time between start and end in microseconds
 */
unsigned int FrameProcessorPlugin::elapsed_us(struct timespec& start, struct timespec& end)
{

  double start_ns = ((double) start.tv_sec * 1000000000) + start.tv_nsec;
  double end_ns = ((double) end.tv_sec * 1000000000) + end.tv_nsec;

  return (unsigned int)((end_ns - start_ns) / 1000);
}

} /* namespace FrameProcessor */
