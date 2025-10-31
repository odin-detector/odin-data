/*
 * FrameProcessorPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FrameProcessorPlugin_H_
#define TOOLS_FILEWRITER_FrameProcessorPlugin_H_

#include <boost/thread.hpp>
#include <unordered_map>

#include "IFrameCallback.h"
#include "IVersionedObject.h"
#include "MetaMessage.h"
#include "IpcMessage.h"
#include "IpcChannel.h"
#include "MetaMessagePublisher.h"
#include "Frame.h"
#include "EndOfAcquisitionFrame.h"
#include "CallDuration.h"
#include "ParamMetadata.h"

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
  std::string get_name() const;
  void set_error(const std::string& msg);
  void set_warning(const std::string& msg);
  void clear_errors();
  virtual bool reset_statistics();
  std::vector<std::string> get_errors();
  std::vector<std::string> get_warnings();
  virtual void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  virtual void requestConfiguration(OdinData::IpcMessage& reply);

  /** Request the plugin's configuration and status Metadata.
   * \param[out] reply - Response IpcMessage with current config metadata.
   */
  __attribute__((always_inline)) void request_configuration_metadata(OdinData::IpcMessage& reply) const{
    auto end = config_metadata_.end();
    for(auto itr = config_metadata_.begin(); itr != end; ++itr) {
      add_metadata(reply, *itr);
    }
  }
  __attribute__((always_inline)) void request_status_metadata(OdinData::IpcMessage& reply) const{
    auto end = status_metadata_.end();
    for(auto itr = status_metadata_.begin(); itr != end; ++itr) {
      add_metadata(reply, *itr);
    }
  }
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

  using ParameterMetadataMap_t = std::unordered_map<std::string, ParamMetadata>;
  /**
   * Helper functions construct the hash_map's elements in-place
   * \param[in] param: key for the ParamMetadata value-object. it is moved to trigger the move-semantics of the key-element in the pair
   * \param[in] type, access_mode, allowed_values, min, max: arguments for ParamMetadata's constructor.They are not moved since the constructor
   *                                                         of ParamMetadata accepts lvalues as references.
   */
  
  auto add_config_param_metadata(std::string param, std::string type, std::string access_mode, std::vector<ParamMetadata::allowed_values_t>& allowed_values)->void {
    config_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET)
    );
  }

  auto add_config_param_metadata(std::string param, std::string type, std::string access_mode, std::vector<ParamMetadata::allowed_values_t>&& allowed_values)->void {
    config_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET)
    );
  }

  auto add_config_param_metadata(std::string param, std::string type, std::string access_mode, int32_t min, int32_t max)->void {
    std::vector<ParamMetadata::allowed_values_t>allowed_vals{}; // This allows us to forward the vector as an lvalue reference to the constructor
    config_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_vals, min, max)
    );
  }

  auto add_status_param_metadata(std::string param, std::string type, std::string access_mode, std::vector<ParamMetadata::allowed_values_t>& allowed_values)->void {
    status_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET)
    );
  }

  auto add_status_param_metadata(std::string param, std::string type, std::string access_mode, std::vector<ParamMetadata::allowed_values_t>&& allowed_values)->void {
    status_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET)
    );
  }

  auto add_status_param_metadata(std::string param, std::string type, std::string access_mode, int32_t min, int32_t max)->void {
    std::vector<ParamMetadata::allowed_values_t>allowed_vals{}; // This allows us to forward the vector as an lvalue reference to the constructor
    status_metadata_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(std::move(param)),
                             std::forward_as_tuple(type, access_mode, allowed_vals, min, max)
    );
  }


private:
  /** Pointer to logger */
  LoggerPtr logger_;

  ParameterMetadataMap_t config_metadata_;
  ParameterMetadataMap_t status_metadata_;

  /** Metadata helper function (implicitly inlined)
   *  \param [in]  metadata - metadata struct to be read from
   *  \param [out] message  - IpcMessage to be appended with metadata
   */
  void add_metadata(OdinData::IpcMessage& message, const ParameterMetadataMap_t::value_type& metadata) const
  {
    std::string param_prefix;
    param_prefix.reserve(128);
    param_prefix = "metadata/" + this->get_name() + '/' + metadata.first + '/';
    message.set_param(param_prefix +  "type", metadata.second.type_);
    message.set_param(param_prefix +  "access_mode", metadata.second.access_mode_);
    if(metadata.second.min_ == ParamMetadata::MIN_UNSET){
      message.set_param(param_prefix +  "min", metadata.second.min_);
    }
    if(metadata.second.max_ == ParamMetadata::MAX_UNSET){
      message.set_param(param_prefix +  "max", metadata.second.max_);
    }

    param_prefix +=  "allowed_values[]";

    auto itr = metadata.second.allowed_values_.begin();
    auto end = metadata.second.allowed_values_.end();
    for (; itr != end; ++itr) {
      switch (itr->which()) {
        case 1:
          message.set_param(param_prefix, boost::get<std::string>(*itr));
          break;
        case 2:
          message.set_param(param_prefix, boost::get<int>(*itr));
          break;
        default:
          return;
      };
    }
  }

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
