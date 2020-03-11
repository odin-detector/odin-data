/*
 * FrameProcessorPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FrameProcessorPlugin_H_
#define TOOLS_FILEWRITER_FrameProcessorPlugin_H_

#include "IFrameCallback.h"
#include "IVersionedObject.h"
#include "MetaMessage.h"
#include "IpcMessage.h"
#include "IpcChannel.h"
#include "MetaMessagePublisher.h"
#include "Frame.h"

namespace FrameProcessor
{

/** Abstract plugin class, providing the IFrameCallback interface.
 *
 * All frame processor plugins must subclass this class. It provides the
 * IFrameCallback interface and associated WorkQueue for transferring
 * Frame objects between plugins. It also provides methods for configuring
 * plugins and for retrieving status from plugins.
 */
class FrameProcessorPlugin : public IFrameCallback, public OdinData::IVersionedObject, public MetaMessagePublisher
{
public:
  FrameProcessorPlugin();
  virtual ~FrameProcessorPlugin();
  void set_name(const std::string& name);
  std::string get_name();
  void set_error(const std::string& msg);
  void clear_errors();
  virtual bool reset_statistics();
  std::vector<std::string> get_errors();
  virtual void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  virtual void requestConfiguration(OdinData::IpcMessage& reply);
  virtual void status(OdinData::IpcMessage& status);
  void add_performance_stats(OdinData::IpcMessage& status);
  void reset_performance_stats();
  void version(OdinData::IpcMessage& status);
  void register_callback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking=false);
  void remove_callback(const std::string& name);
  void remove_all_callbacks();

protected:
  void push(boost::shared_ptr<Frame> frame);

private:
  /** Pointer to logger */
  LoggerPtr logger_;

  void callback(boost::shared_ptr<Frame> frame);

  /**
   * This is called by the callback method when any new frames have
   * arrived and must be overridden by child classes.
   *
   * \param[in] frame - Pointer to the frame.
   */
  virtual void process_frame(boost::shared_ptr<Frame> frame) = 0;

  /** Name of this plugin */
  std::string name_;
  /** Map of registered plugins for callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
  /** Map of registered plugins for blocking callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > blocking_callbacks_;
  /** Error message array*/
  std::vector<std::string> error_messages_;
  /** Last process time */
  uint64_t last_process_time_;
  /** Maximum process time since last reset */
  uint64_t max_process_time_;
  /** Exp average process time since last reset */
  double average_process_time_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_FrameProcessorPlugin_H_ */
