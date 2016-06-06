/*
 * FileWriterPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include <FileWriterPlugin.h>

namespace filewriter
{

  FileWriterPlugin::FileWriterPlugin ()
  {
    // TODO Auto-generated constructor stub

  }

  FileWriterPlugin::~FileWriterPlugin ()
  {
    // TODO Auto-generated destructor stub
  }

  boost::shared_ptr<filewriter::JSONMessage> FileWriterPlugin::configure(boost::shared_ptr<filewriter::JSONMessage> config)
  {
    // Default method simply returns the configuration that was passed in
    return config;
  }

  void FileWriterPlugin::callback(boost::shared_ptr<Frame> frame)
  {
    // TODO Currently this is a stub, simply call process frame
    this->processFrame(frame);
  }

} /* namespace filewriter */
