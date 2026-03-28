/*
 *  Created on: 22 Nov 2021
 *      Author: Gary Yendell
 */
#include "ParameterPublishPlugin.h"
#include "DebugLevelLogger.h"
#include "Json.h"
#include "version.h"

namespace FrameProcessor {
const std::string ParameterPublishPlugin::CONFIG_ENDPOINT = "endpoint";
const std::string ParameterPublishPlugin::CONFIG_ADD_PARAMETER = "add_parameter";
const std::string ParameterPublishPlugin::DATA_FRAME_NUMBER = "frame_number";
const std::string ParameterPublishPlugin::DATA_PARAMETERS = "parameters";
/**
 * The constructor sets up logging used within the class.
 */
ParameterPublishPlugin::ParameterPublishPlugin() :
    publish_channel_(ZMQ_PUB),
    channel_endpoint_("")
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.ParameterPublishPlugin");
    LOG4CXX_INFO(logger_, "ParameterPublishPlugin version " << this->get_version_long() << " loaded");
    add_config_param_metadata(CONFIG_ENDPOINT, PMDD::STRING_T, PMDA::READ_WRITE);
    add_config_param_metadata(DATA_FRAME_NUMBER, PMDD::INT_T, PMDA::READ_ONLY, 0);
    add_config_param_metadata(DATA_PARAMETERS, PMDD::STRINGARR_T, PMDA::READ_WRITE);
}

/**
 * Destructor.
 */
ParameterPublishPlugin::~ParameterPublishPlugin()
{
    this->publish_channel_.close();
    LOG4CXX_TRACE(logger_, "ParameterPublishPlugin destructor.");
}

/**
 * Publish the configured parameters if found on the frame
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void ParameterPublishPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
    try {
        OdinData::JsonDict json;
        OdinData::JsonDict parameters_json;
        for (auto& parameter : this->parameters_) {
            if (frame->meta_data().has_parameter(parameter)) {
                parameters_json.add(parameter, frame->meta_data().get_parameter<uint64_t>(parameter));
            }
        }
        json.add(DATA_FRAME_NUMBER, static_cast<int64_t>(frame->get_frame_number()));
        std::string&& str = parameters_json.str();
        json.add(DATA_PARAMETERS, str);
        this->publish_channel_.send(json.str().c_str());
    } catch (std::bad_alloc& e) {
        LOG4CXX_ERROR(
            logger_,
            "Error: RapidJSON - " << e.what() << " frame - " << frame->get_frame_number() << " acquisitionID - "
                                  << frame->get_meta_data().get_acquisition_ID()
        );
    } catch (zmq::error_t& e) {
        LOG4CXX_ERROR(
            logger_,
            "Error: ZMQ error: " << e.what() << " frame - " << frame->get_frame_number() << " acquisitionID - "
                                 << frame->get_meta_data().get_acquisition_ID()
        );
    }
    this->push(frame);
}

/**
 * Set configuration options for this Plugin.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void ParameterPublishPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
    try {
        if (config.has_param(CONFIG_ENDPOINT)) {
            this->setup_publish_channel(config.get_param<std::string>(CONFIG_ENDPOINT));
        }

        if (config.has_param(CONFIG_ADD_PARAMETER)) {
            this->parameters_.insert(config.get_param<const rapidjson::Value&>(CONFIG_ADD_PARAMETER).GetString());
        }
    } catch (std::runtime_error& e) {
        std::stringstream ss;
        ss << "Bad ctrl msg: " << e.what();
        this->set_error(ss.str());
        throw;
    }
}

/**
 * Get the configuration values for this plugin
 *
 * \param[out] reply - Response IpcMessage.
 */
void ParameterPublishPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
    reply.set_param(get_name() + "/" + CONFIG_ENDPOINT, this->channel_endpoint_);
    // Create use a key with a `[]` suffix so that it appends to an array as we set it repeatedly
    std::string parameters_key = get_name() + "/" + DATA_PARAMETERS + "[]";
    for (auto& it : this->parameters_) {
        reply.set_param(parameters_key, it);
    }
}

/** Bind to endpoint and store for config reporting
 *
 * \param[in] endpoint - The endpoint to bind to
 */
void ParameterPublishPlugin::setup_publish_channel(std::string&& endpoint)
{
    try {
        if (!this->channel_endpoint_.empty()) {
            publish_channel_.unbind(this->channel_endpoint_.c_str());
        }
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Connecting channel to endpoint: " << endpoint);
        this->channel_endpoint_ = std::move(endpoint);
        this->publish_channel_.bind(this->channel_endpoint_.c_str());
    } catch (zmq::error_t& e) {
        throw std::runtime_error(e.what());
    }
}

/** Version reporting
 *
 */

int ParameterPublishPlugin::get_version_major()
{
    return ODIN_DATA_VERSION_MAJOR;
}

int ParameterPublishPlugin::get_version_minor()
{
    return ODIN_DATA_VERSION_MINOR;
}

int ParameterPublishPlugin::get_version_patch()
{
    return ODIN_DATA_VERSION_PATCH;
}

std::string ParameterPublishPlugin::get_version_short()
{
    return ODIN_DATA_VERSION_STR_SHORT;
}

std::string ParameterPublishPlugin::get_version_long()
{
    return ODIN_DATA_VERSION_STR;
}
} /* namespace FrameProcessor */
