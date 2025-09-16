/*
 * FrameProcessorPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FrameProcessorPlugin_H_
#define TOOLS_FILEWRITER_FrameProcessorPlugin_H_

#include <boost/thread.hpp>

#include "IFrameCallback.h"
#include "IVersionedObject.h"
#include "MetaMessage.h"
#include "IpcMessage.h"
#include "IpcChannel.h"
#include "MetaMessagePublisher.h"
#include "Frame.h"
#include "EndOfAcquisitionFrame.h"
#include "CallDuration.h"

namespace FrameProcessor
{
// represents json null type
struct JsonNullType{};

/** This struct is a representation of the metadata
*/
struct ParamMetadata
{
  using param_metadata_vec = std::vector<boost::variant<std::string, int>>;
  const std::string path;
  const std::string type;
  const std::string access_mode;
  const param_metadata_vec allowed_values;
  const int32_t min;
  const int32_t max;
  const bool has_min;
  const bool has_max;
};

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
  std::string get_name() const;
  void set_error(const std::string& msg);
  void set_warning(const std::string& msg);
  void clear_errors();
  virtual bool reset_statistics();
  std::vector<std::string> get_errors();
  std::vector<std::string> get_warnings();
  virtual void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  virtual void requestConfiguration(OdinData::IpcMessage& reply);
  void requestConfigurationMetadata(OdinData::IpcMessage& reply) const;
  void requestStatusMetadata(OdinData::IpcMessage& reply) const;
  virtual void execute(const std::string& command, OdinData::IpcMessage& reply);
  virtual std::vector<std::string> requestCommands();
  virtual void status(OdinData::IpcMessage& status);
  void add_performance_stats(OdinData::IpcMessage& status);
  void reset_performance_stats();
  void version(OdinData::IpcMessage& status);
  void register_callback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking=false);
  void remove_callback(const std::string& name);
  void remove_all_callbacks();
  void notify_end_of_acquisition();

protected:
  void push(boost::shared_ptr<Frame> frame);
  void push(const std::string& plugin_name, boost::shared_ptr<Frame> frame);

private:
  /** Pointer to logger */
  LoggerPtr logger_;



  /** Metadata helper function (implicitly inlined)
   *  \param [in]  metadata - metadata struct to be read from
   *  \param [out] message  - IpcMessage to be appended with metadata
   */
  void add_metadata(OdinData::IpcMessage& message, const ParamMetadata& metadata) const
  {
    std::string str = get_name() + metadata.path + '/';
    message.set_param(str +  "/type", metadata.type);
    message.set_param(str +  "/access_mode", metadata.access_mode);
    if(metadata.has_min){
      message.set_param(str +  "/min", metadata.min);
    }
    if(metadata.has_max){
      message.set_param(str +  "/max", metadata.max);
    }

    str +=  "/allowed_values[]";

    auto first = metadata.allowed_values.begin();
    auto end = metadata.allowed_values.end();
    for (; first != end; ++first) {
      switch (first->which()) {
        case 0:
          message.set_param(str, boost::get<std::string>(*first));
          break;
        case 1:
          message.set_param(str, boost::get<int>(*first));
          break;
        default:
          return;
      };
    }
  }

  /** These is a private virtual methods
   *  MUST be customized by every derived FrameProcessor 
   *  plugin class that can append metadata.
   *  It is essentially something in guise of a factory method.
   * \returns a vector of ParamMetadata
   */
  virtual std::vector<ParamMetadata>& get_config_metadata() const noexcept;
  virtual std::vector<ParamMetadata>& get_status_metadata() const noexcept;

  void callback(boost::shared_ptr<Frame> frame);

  /**
   * This is called by the callback method when any new frames have
   * arrived and must be overridden by child classes.
   *
   * \param[in] frame - Pointer to the frame.
   */
  virtual void process_frame(boost::shared_ptr<Frame> frame) = 0;
  virtual void process_end_of_acquisition();

  /** Name of this plugin */
  std::string name_;
  /** Map of registered plugins for callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
  /** Map of registered plugins for blocking callbacks, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > blocking_callbacks_;
  /** Error message array */
  std::vector<std::string> error_messages_;
  /** Warning message array */
  std::vector<std::string> warning_messages_;
  /** Mutex to make accessing error_messages_ threadsafe */
  boost::mutex mutex_;
  /** process_frame performance stats */
  CallDuration process_duration_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_FrameProcessorPlugin_H_ */
