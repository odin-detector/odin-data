/*
 * FrameProcessorPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include "FrameProcessorPlugin.h"

namespace FrameProcessor
{
  const std::string FrameProcessorPlugin::META_RX_INTERFACE        = "inproc://meta_rx";

  /**
   * Constructor, initialises name_.
   */
  FrameProcessorPlugin::FrameProcessorPlugin() :
      name_(""),
	  metaChannel_(ZMQ_PUSH)
  {
    logger_ = log4cxx::Logger::getLogger("FW.FrameProcessorPlugin");
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
  void FrameProcessorPlugin::setName(const std::string& name)
  {
    // Record our name
    name_ = name;
  }

  /**
   * Get the name of this plugin
   *
   * \return The name.
   */
  std::string FrameProcessorPlugin::getName()
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
  void FrameProcessorPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
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
   * our map, indexed by name.  If the callback already exists within our
   * map then this is a no-op.
   *
   * \param[in] name - Index of the callback (plugin index).
   * \param[in] cb - Pointer to an IFrameCallback interface (plugin).
   */
  void FrameProcessorPlugin::registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking)
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
  void FrameProcessorPlugin::removeCallback(const std::string& name)
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

  void FrameProcessorPlugin::connectMetaChannel()
  {
    metaChannel_.connect(META_RX_INTERFACE.c_str());
  }

  /** Publish meta data from this plugin.
   *
   * \param[in] item - Name of the meta data item to publish.
   * \param[in] value - The value of the meta data item to publish.
   * \param[in] header - Optional additional header data to publish.
   */
  void FrameProcessorPlugin::publishMeta(const std::string& item, int32_t value, const std::string &header)
  {
	  // Create a new MetaMessage object and send to the consumer
	  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name_, item, "integer", header, sizeof(int32_t), &value);
	  // We need the pointer to the object cast to be able to pass it through ZMQ
	  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
	  // Send the pointer value to the listener
	  metaChannel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
  }

  /**
   * Publish meta data from this plugin.
   *
   * \param[in] item - Name of the meta data item to publish.
   * \param[in] value - The value of the meta data item to publish.
   * \param[in] header - Optional additional header data to publish.
   */
  void FrameProcessorPlugin::publishMeta(const std::string& item, double value, const std::string& header)
  {
	  // Create a new MetaMessage object and send to the consumer
	  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name_, item, "double", header, sizeof(double), &value);
	  // We need the pointer to the object cast to be able to pass it through ZMQ
	  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
	  // Send the pointer value to the listener
	  metaChannel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
  }

  /**
   * Publish meta data from this plugin.
   *
   * \param[in] item - Name of the meta data item to publish.
   * \param[in] value - The value of the meta data item to publish.
   * \param[in] header - Optional additional header data to publish.
   */
  void FrameProcessorPlugin::publishMeta(const std::string& item, const std::string& value, const std::string& header)
  {
	  // Create a new MetaMessage object and send to the consumer
	  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name_, item, "string", header, value.length(), value.c_str());
	  // We need the pointer to the object cast to be able to pass it through ZMQ
	  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
	  // Send the pointer value to the listener
	  metaChannel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
  }

  /**
   * Publish meta data from this plugin.
   *
   * \param[in] item - Name of the meta data item to publish.
   * \param[in] pValue - A pointer to the meta data value.  The memory will be copied.
   * \param[in] length - The size (in bytes) of the meta data value.
   * \param[in] header - Optional additional header data to publish.
   */
  void FrameProcessorPlugin::publishMeta(const std::string& item, const void *pValue, size_t length, const std::string& header)
  {
	  // Create a new MetaMessage object and send to the consumer
	  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name_, item, "raw", header, length, pValue);
	  // We need the pointer to the object cast to be able to pass it through ZMQ
	  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
	  // Send the pointer value to the listener
	  metaChannel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
  }


  /**
   * We have been called back with a frame from a plugin that we registered
   * with.  This method calls the processFrame pure virtual method that
   * must be overridden by any children of this abstract class.
   *
   * \param[in] frame - Pointer to the frame.
   */
  void FrameProcessorPlugin::callback(boost::shared_ptr<Frame> frame)
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
  void FrameProcessorPlugin::push(boost::shared_ptr<Frame> frame)
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

} /* namespace FrameProcessor */
