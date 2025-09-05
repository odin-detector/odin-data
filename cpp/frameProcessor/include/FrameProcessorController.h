/*
 * FrameProcessorController.h
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FrameProcessorController_H_
#define TOOLS_FILEWRITER_FrameProcessorController_H_

#include <boost/shared_ptr.hpp>
#include <log4cxx/logger.h>

#include "logging.h"
#include "IpcReactor.h"
#include "IpcChannel.h"
#include "SharedMemoryController.h"
#include "SharedBufferManager.h"
#include "ClassLoader.h"
#include "FrameProcessorPlugin.h"
#include "OdinDataDefaults.h"

namespace FrameProcessor
{

/**
 * The FrameProcessorController class has overall responsibility for management of the
 * core classes and plugins present within the frame processor application. This class
 * maintains the SharedMemoryController and SharedMemoryParser classes. The class
 * also manages the control IpcChannel, and accepts configuration IpcMessages. The
 * class provides an interface for loading plugins, connecting the plugins together
 * into chains and for configuring the plugins (from the control channel).
 *
 * The class uses an IpcReactor to manage connections and status updates.
 */
class FrameProcessorController : public IFrameCallback,
                                 public boost::enable_shared_from_this<FrameProcessorController>
{
public:
  FrameProcessorController(unsigned int num_io_threads=OdinData::Defaults::default_io_threads);
  virtual ~FrameProcessorController();
  void handleCtrlChannel();
  void handleMetaRxChannel();
  void provideStatus(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void provideVersion(OdinData::IpcMessage& reply);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestConfiguration(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void execute(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestCommands(OdinData::IpcMessage& reply);
  void resetStatistics(OdinData::IpcMessage& reply);
  void configurePlugin(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void loadPlugin(const std::string& index, const std::string& name, const std::string& library);
  void connectPlugin(const std::string& index, const std::string& connectTo);
  void disconnectPlugin(const std::string& index, const std::string& disconnectFrom);
  void disconnectAllPlugins();
  void run();
  void waitForShutdown();
  void shutdown();
private:
  /** Configuration constant for the meta-data Rx interface **/
  static const std::string META_RX_INTERFACE;

  /** Configuration constant to shutdown the frame processor **/
  static const std::string CONFIG_SHUTDOWN;

  /** Configuration constant to inject an End Of Acquisition frame into the plugin chain **/
  static const std::string CONFIG_EOA;

  /** Configuration constant to set the debug level of the frame processor **/
  static const std::string CONFIG_DEBUG;

  /** Configuration constant for name of shared memory storage **/
  static const std::string CONFIG_FR_SHARED_MEMORY;
  /** Configuration constant for connection string for frame release **/
  static const std::string CONFIG_FR_RELEASE;
  /** Configuration constant for connection string for frame ready **/
  static const std::string CONFIG_FR_READY;
  /** Configuration constant for executing setup of shared memory interface **/
  static const std::string CONFIG_FR_SETUP;

  /** Configuration constant for control socket endpoint **/
  static const std::string CONFIG_CTRL_ENDPOINT;
  /** Configuration constant for meta data endpoint **/
  static const std::string CONFIG_META_ENDPOINT;

  /** Configuration constant for plugin related items **/
  static const std::string CONFIG_PLUGIN;
  /** Configuration constant for listing loaded plugins **/
  static const std::string CONFIG_PLUGIN_LOAD;
  /** Configuration constant for connecting plugins **/
  static const std::string CONFIG_PLUGIN_CONNECT;
  /** Configuration constant for disconnecting plugins **/
  static const std::string CONFIG_PLUGIN_DISCONNECT;
  /** Configuration keyword for disconnecting all plugins **/
  static const std::string CONFIG_PLUGIN_DISCONNECT_ALL;
  /** Configuration constant for a plugin name **/
  static const std::string CONFIG_PLUGIN_NAME;
  /** Configuration constant for a plugin index **/
  static const std::string CONFIG_PLUGIN_INDEX;
  /** Configuration constant for a plugin external library **/
  static const std::string CONFIG_PLUGIN_LIBRARY;
  /** Configuration constant for setting up a plugin connection **/
  static const std::string CONFIG_PLUGIN_CONNECTION;

  /** Configuration constant for storing a named configuration object **/
  static const std::string CONFIG_STORE;
  /** Configuration constant for executing a named configuration object **/
  static const std::string CONFIG_EXECUTE;
  /** Configuration constant for the name of a stored configuration object **/
  static const std::string CONFIG_INDEX;
  /** Configuration constant for the value of a stored configuration object **/
  static const std::string CONFIG_VALUE;

  /** Configuration constant for executing a command **/
  static const std::string COMMAND_KEY;
  /** Configuration constant for obtaining a list of supported commands **/
  static const std::string SUPPORTED_KEY;

  /** Configuration constant for the meta TX channel high water mark **/
  static const int META_TX_HWM;

  void setupFrameReceiverInterface(const std::string& frPublisherString,
                                   const std::string& frSubscriberString);
  void closeFrameReceiverInterface();
  void setupControlInterface(const std::string& ctrlEndpointString);
  void closeControlInterface();
  void setupMetaRxInterface();
  void closeMetaRxInterface();
  void setupMetaTxInterface(const std::string& metaEndpointString);
  void closeMetaTxInterface();
  void runIpcService(void);
  void tickTimer(void);
  void callback(boost::shared_ptr<Frame> frame);

  /** Pointer to the logging facility */
  log4cxx::LoggerPtr                                              logger_;
  /** Pointer to the shared memory controller instance for this process */
  boost::shared_ptr<SharedMemoryController>                       sharedMemController_;
  /** Map of plugins loaded, indexed by plugin index */
  std::map<std::string, boost::shared_ptr<FrameProcessorPlugin> > plugins_;
  /** Map of stored configuration objects */
  std::map<std::string, std::string>                              stored_configs_;
  /** Condition for exiting this file writing process */
  boost::condition_variable                                       exitCondition_;
  /** Frames to write before shutting down - 0 to disable shutdown */
  int                                                             shutdownFrameCount;
  /** Total frames processed */
  int                                                             totalFrames;
  /** Master frame specifier - Frame to include in count of total frames processed */
  std::string                                                     masterFrame;
  /** Mutex used for locking the exitCondition */
  boost::mutex                                                    exitMutex_;
  /** Used to check for Ipc tick timer termination */
  bool                                                            runThread_;
  /** Is the main thread running */
  bool                                                            threadRunning_;
  /** Did an error occur during the thread initialisation */
  bool                                                            threadInitError_;
  /** Have we sent sent the shutdown command to the plugins */
  bool                                                            pluginShutdownSent_;
  /** Have we successfully shutdown */
  bool                                                            shutdown_;
  /** Main thread used for control message handling */
  boost::thread                                                   ctrlThread_;
  /** Store for any messages occurring during thread initialisation */
  std::string                                                     threadInitMsg_;
  /** Pointer to the IpcReactor for incoming frame handling */
  boost::shared_ptr<OdinData::IpcReactor>                         reactor_;
  /** End point for control messages */
  std::string                                                     ctrlChannelEndpoint_;
  /** ZMQ context for IPC channels */
  OdinData::IpcContext& ipc_context_;
  /** IpcChannel for control messages */
  OdinData::IpcChannel                                            ctrlChannel_;
  /** IpcChannel for meta-data messages */
  OdinData::IpcChannel                                            metaRxChannel_;
  /** End point for publishing meta-data messages */
  std::string                                                     metaTxChannelEndpoint_;
  /** IpcChannel for publishing meta-data messages */
  OdinData::IpcChannel                                            metaTxChannel_;
  /** End point for frameReceiver ready channel */
  std::string                                                     frReadyEndpoint_;
  /** End point for frameReceiver release channel */
  std::string                                                     frReleaseEndpoint_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_FrameProcessorController_H_ */
