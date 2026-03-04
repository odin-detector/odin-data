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

#include "CallDuration.h"
#include "EndOfAcquisitionFrame.h"
#include "Frame.h"
#include "IFrameCallback.h"
#include "IVersionedObject.h"
#include "IpcChannel.h"
#include "IpcMessage.h"
#include "MetaMessage.h"
#include "MetaMessagePublisher.h"
#include "ParamMetadata.h"

namespace FrameProcessor {

/** Abstract plugin class, providing the IFrameCallback interface.
 *
 * All frame processor plugins must subclass this class. It provides the
 * IFrameCallback interface and associated WorkQueue for transferring
 * Frame objects between plugins. It also provides methods for configuring
 * plugins and for retrieving status from plugins.
 */
class FrameProcessorPlugin : public IFrameCallback, public OdinData::IVersionedObject, public MetaMessagePublisher {
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
    __attribute__((always_inline)) void request_configuration_metadata(OdinData::IpcMessage& reply) const
    {
        auto end = config_metadata_.end();
        for (auto itr = config_metadata_.begin(); itr != end; ++itr) {
            add_metadata(reply, *itr);
        }
    }
    __attribute__((always_inline)) void request_status_metadata(OdinData::IpcMessage& reply) const
    {
        auto end = status_metadata_.end();
        for (auto itr = status_metadata_.begin(); itr != end; ++itr) {
            add_metadata(reply, *itr);
        }
    }
    virtual void execute(const std::string& command, OdinData::IpcMessage& reply);
    virtual std::vector<std::string> requestCommands();
    virtual void status(OdinData::IpcMessage& status);
    void add_performance_stats(OdinData::IpcMessage& status);
    void reset_performance_stats();
    void version(OdinData::IpcMessage& status);
    void register_callback(const std::string& name, boost::shared_ptr<IFrameCallback> cb, bool blocking = false);
    void remove_callback(const std::string& name);
    void remove_all_callbacks();
    void notify_end_of_acquisition();

protected:
    void push(boost::shared_ptr<Frame> frame);
    void push(const std::string& plugin_name, boost::shared_ptr<Frame> frame);

    using ParameterMetadataMap_t = std::unordered_map<std::string, ParamMetadata>;
    using All_val_vec_t = std::vector<ParamMetadata::allowed_values_t>;

    // template type traits alias to enforce the DataType passed into function
    template <typename TYPE>
    using is_all_vals_vec = std::is_same<typename std::decay<TYPE>::type, All_val_vec_t>;
    template <typename TYPE>
    using is_datatype = std::is_same<typename std::decay<TYPE>::type, ParamMetadata::Datatype>;
    template <typename TYPE>
    using is_str_conv = std::is_convertible<TYPE, std::string>; // can convert TYPE o std::string
    template <typename DataType_, typename Allowed_>
    using enableif_t = typename std::enable_if<(is_str_conv<DataType_>::value || is_datatype<DataType_>::value) && is_all_vals_vec<Allowed_>::value>::type;
    template <typename DataType_>
    using enableif_str_or_datatype_t = typename std::enable_if<is_str_conv<DataType_>::value || is_datatype<DataType_>::value>::type;

    /**
     * Helper functions construct the hash_map's elements in-place
     * \param[in] param: key for the ParamMetadata value-object. it is moved to trigger the move-semantics
     *                                    of the key-element in the pair
     * \param[in] access_mode, min, max: arguments for ParamMetadata's constructor. They are not moved
     *                                    since the constructor of ParamMetadata accepts lvalues as references.
     * \param[in] type:           For this argument, the template function ONLY allows std::string
     *                                    convertible types OR ParamMetadata::Datatype enum values to be passed im!
     * \param[in] allowed_values: For this argument, the template function ONLY allows
     *                                    std::vector<ParamMetadata::allowed_values_t> types to be passed in!
     *
     * Illustrative Usecase in YourPlugin.
     * Where YourPlugin::CONFIG_PARAM[N] is your parameter's string:
     *
     *    using PMD  = struct ParamMetadata;
     *    using PMDA = PMD::AccessMode; // Alias for the AccessMode enums scope
     *    using PMDD = PMD::Datatype;   // Alias for the Datatype enums scope
     *
     ***  // 2nd overload is instantiated 'min', 'max' and 'allowed_values' ALL UNSET state
     *    add_config_param_metadata(YourPlugin::CONFIG_PARAM1, PMDD::STRING_T, PMDA::READ_WRITE);
     *
     ***  // 1st overload is instantiated NOTE that a custom string - ("DOUBLE_ARRAY"), was passed to DataType argument
     *    // allowed_values argument is: {"align1", "align2"}.
     *    add_config_param_metadata("MY_PLUGIN_PARAM_STRING", "DOUBLE_ARRAY", PMDA::READ_ONLY, {"align1", "align2"});
     *
     ***  // 2nd overload is instantiated only 'min' is set to 1, 'max' and 'allowed_values' UNSET state
     *    add_config_param_metadata(YourPlugin::CONFIG_PARAM3, PMDD::UINT_T, PMDA::READ_WRITE, 1);
     */
    template <typename DataType, typename Allowed_ = All_val_vec_t, typename = enableif_t<DataType, Allowed_>>
    auto add_config_param_metadata(std::string param, DataType type, ParamMetadata::AccessMode access_mode, Allowed_&& allowed_values) -> void
    {
        config_metadata_.emplace(std::piecewise_construct,
            std::forward_as_tuple(std::move(param)),
            std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET));
    }

    template <typename DataType, typename = enableif_str_or_datatype_t<DataType>>
    auto add_config_param_metadata(std::string param,
        DataType type,
        ParamMetadata::AccessMode access_mode,
        int32_t min = ParamMetadata::MIN_UNSET,
        int32_t max = ParamMetadata::MAX_UNSET) -> void
    {
        std::vector<ParamMetadata::allowed_values_t> allowed_vals {}; // This allows us to forward the vector as an lvalue reference to the constructor
        config_metadata_.emplace(std::piecewise_construct,
            std::forward_as_tuple(std::move(param)),
            std::forward_as_tuple(type, access_mode, allowed_vals, min, max));
    }

    template <typename DataType, typename Allowed_ = All_val_vec_t, typename = enableif_t<DataType, Allowed_>>
    auto add_status_param_metadata(std::string param, DataType type, ParamMetadata::AccessMode access_mode, Allowed_&& allowed_values) -> void
    {
        status_metadata_.emplace(std::piecewise_construct,
            std::forward_as_tuple(std::move(param)),
            std::forward_as_tuple(type, access_mode, allowed_values, ParamMetadata::MIN_UNSET, ParamMetadata::MAX_UNSET));
    }

    template <typename DataType, typename = enableif_str_or_datatype_t<DataType>>
    auto add_status_param_metadata(std::string param,
        DataType type,
        ParamMetadata::AccessMode access_mode,
        int32_t min = ParamMetadata::MIN_UNSET,
        int32_t max = ParamMetadata::MAX_UNSET) -> void
    {
        std::vector<ParamMetadata::allowed_values_t> allowed_vals {}; // This allows us to forward the vector as an lvalue reference to the constructor
        status_metadata_.emplace(std::piecewise_construct,
            std::forward_as_tuple(std::move(param)),
            std::forward_as_tuple(type, access_mode, allowed_vals, min, max));
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
        message.set_param(param_prefix + "type", metadata.second.datatype_as_string());
        message.set_param(param_prefix + "access_mode", metadata.second.access_mode_as_string());
        if (metadata.second.min_ == ParamMetadata::MIN_UNSET) {
            message.set_param(param_prefix + "min", metadata.second.min_);
        }
        if (metadata.second.max_ == ParamMetadata::MAX_UNSET) {
            message.set_param(param_prefix + "max", metadata.second.max_);
        }

        param_prefix += "allowed_values[]";

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
    std::map<std::string, boost::shared_ptr<IFrameCallback>> callbacks_;
    /** Map of registered plugins for blocking callbacks, indexed by name */
    std::map<std::string, boost::shared_ptr<IFrameCallback>> blocking_callbacks_;
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
