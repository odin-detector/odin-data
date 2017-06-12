/*
 * FrameProcessorPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FrameProcessorPlugin_H_
#define TOOLS_FILEWRITER_FrameProcessorPlugin_H_

#include "IFrameCallback.h"
#include "MetaMessage.h"
#include "IpcMessage.h"
#include "IpcChannel.h"

namespace FrameProcessor
{

/** Abstract plugin class, providing the IFrameCallback interface.
 *
 * All frame processor plugins must subclass this class. It provides the
 * IFrameCallback interface and associated WorkQueue for transferring
 * Frame objects between plugins. It also provides methods for configuring
 * plugins and for retrieving status from plugins.
 */
class FrameProcessorPlugin : public IFrameCallback
{
public:
  FrameProcessorPlugin();
  virtual ~FrameProcessorPlugin();
  void setName(const std::string& name);
  std::string getName();
  virtual void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  virtual void status(OdinData::IpcMessage& status);
  void registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking=false);
  void removeCallback(const std::string& name);

  void connectMetaChannel();
  void publishMeta(const std::string& item, int32_t value, const std::string& header = "");
  void publishMeta(const std::string& item, double value, const std::string& header = "");
  void publishMeta(const std::string& item, const std::string& value, const std::string& header = "");
  void publishMeta(const std::string& item, const void *pValue, size_t length, const std::string& header = "");

protected:
  void push(boost::shared_ptr<Frame> frame);

private:
  /** Pointer to logger */
  LoggerPtr logger_;
  /** Configuration constant for the meta-data Rx interface **/
  static const std::string META_RX_INTERFACE;

  void callback(boost::shared_ptr<Frame> frame);

  /**
   * This is called by the callback method when any new frames have
   * arrived and must be overridden by child classes.
   *
   * \param[in] frame - Pointer to the frame.
   */
  virtual void processFrame(boost::shared_ptr<Frame> frame) = 0;

  /** Name of this plugin */
  std::string name_;
  /** Map of registered plugins for callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
  /** Map of registered plugins for blocking callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > blockingCallbacks_;
  /** IpcChannel for meta-data messages */
  OdinData::IpcChannel metaChannel_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_FrameProcessorPlugin_H_ */
