/*
 * FileWriterController.h
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_
#define TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_

#include <boost/shared_ptr.hpp>

#include "FileWriter.h"
#include "FileWriterPlugin.h"
#include "IJSONCallback.h"
#include "JSONMessage.h"
#include "JSONPublisher.h"
#include "JSONSubscriber.h"
#include "SharedMemoryController.h"
#include "SharedMemoryParser.h"
#include "ClassLoader.h"

namespace filewriter
{

  class FileWriterController : public IJSONCallback
  {
  public:
    FileWriterController();
    virtual ~FileWriterController();
    void loadPlugin(const std::string& name, const std::string& library);
  private:
    static const std::string CONFIG_LOAD_PLUGIN;
    static const std::string CONFIG_CONNECT_PLUGIN;
    static const std::string CONFIG_DISCONNECT_PLUGIN;
    static const std::string CONFIG_PLUGIN_NAME;
    static const std::string CONFIG_PLUGIN_CONNECTION;
    static const std::string CONFIG_PLUGIN_LIBRARY;

    void callback(boost::shared_ptr<JSONMessage> msg);

    boost::shared_ptr<SharedMemoryController> sharedMemController_;
    boost::shared_ptr<SharedMemoryParser> sharedMemParser_;
    boost::shared_ptr<JSONPublisher> frameReleasePublisher_;
    boost::shared_ptr<JSONSubscriber> frameReadySubscriber_;
    std::map<std::string, boost::shared_ptr<FileWriterPlugin> > plugins_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_ */
