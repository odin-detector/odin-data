/*
 * FileWriterPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include <FileWriterPlugin.h>

namespace filewriter
{

  FileWriterPlugin::FileWriterPlugin() :
      name_("")
  {
  }

  FileWriterPlugin::~FileWriterPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  void FileWriterPlugin::setName(const std::string& name)
  {
    // Record our name
    name_ = name;
  }

  std::string FileWriterPlugin::getName()
  {
    // Return our name
    return name_;
  }

  void FileWriterPlugin::configure(FrameReceiver::IpcMessage& config, FrameReceiver::IpcMessage& reply)
  {
    // Default method simply does nothing
  }

  void FileWriterPlugin::status(FrameReceiver::IpcMessage& status)
  {
    // Default method simply does nothing
  }

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

  void FileWriterPlugin::callback(boost::shared_ptr<Frame> frame)
  {
    // TODO Currently this is a stub, simply call process frame
    this->processFrame(frame);
  }

  void FileWriterPlugin::push(boost::shared_ptr<Frame> frame)
  {
    // Loop over callbacks, placing frame onto each queue
    std::map<std::string, boost::shared_ptr<IFrameCallback> >::iterator cbIter;
    for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
      cbIter->second->getWorkQueue()->add(frame);
    }
  }

} /* namespace filewriter */
