/*
 * FileWriterController.h
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_
#define TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>

#include "FileWriterPlugin.h"
#include "IpcReactor.h"
#include "IpcChannel.h"
#include "SharedMemoryController.h"
#include "SharedMemoryParser.h"
#include "ClassLoader.h"

namespace filewriter
{

  class FileWriterController
  {
  public:
    FileWriterController();
    virtual ~FileWriterController();
    void handleCtrlChannel();
    void configure(FrameReceiver::IpcMessage& config, FrameReceiver::IpcMessage& reply);
    void configurePlugin(FrameReceiver::IpcMessage& config, FrameReceiver::IpcMessage& reply);
    void loadPlugin(const std::string& index, const std::string& name, const std::string& library);
    void connectPlugin(const std::string& index, const std::string& connectTo);
    void disconnectPlugin(const std::string& index, const std::string& disconnectFrom);
    void waitForShutdown();
  private:
    static const std::string CONFIG_SHUTDOWN;

    static const std::string CONFIG_FR_SHARED_MEMORY; // Name of shared memory storage
    static const std::string CONFIG_FR_RELEASE;       // Cnxn string for frame release
    static const std::string CONFIG_FR_READY;         // Cnxn string for frame ready
    static const std::string CONFIG_FR_SETUP;         // Command to execute setup

    static const std::string CONFIG_CTRL_ENDPOINT;    // Command to start the control socket

    static const std::string CONFIG_PLUGIN;
    static const std::string CONFIG_PLUGIN_LIST;

    static const std::string CONFIG_LOAD_PLUGIN;
    static const std::string CONFIG_CONNECT_PLUGIN;
    static const std::string CONFIG_DISCONNECT_PLUGIN;
    static const std::string CONFIG_PLUGIN_NAME;
    static const std::string CONFIG_PLUGIN_INDEX;
    static const std::string CONFIG_PLUGIN_CONNECT_TO;
    static const std::string CONFIG_PLUGIN_DISCONNECT_FROM;
    static const std::string CONFIG_PLUGIN_LIBRARY;

    void setupFrameReceiverInterface(const std::string& sharedMemName,
                                     const std::string& frPublisherString,
                                     const std::string& frSubscriberString);
    void setupControlInterface(const std::string& ctrlEndpointString);
    void runIpcService(void);
    void tickTimer(void);

    log4cxx::LoggerPtr logger_;
    boost::shared_ptr<SharedMemoryController> sharedMemController_;
    boost::shared_ptr<SharedMemoryParser> sharedMemParser_;
    std::map<std::string, boost::shared_ptr<FileWriterPlugin> > plugins_;
    boost::condition_variable exitCondition_;
    boost::mutex exitMutex_;

    bool                                         runThread_;
    bool                                         threadRunning_;
    bool                                         threadInitError_;
    boost::thread                                ctrlThread_;
    std::string                                  threadInitMsg_;
    boost::shared_ptr<FrameReceiver::IpcReactor> reactor_;
    FrameReceiver::IpcChannel                    ctrlChannel_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_FILEWRITERCONTROLLER_H_ */
