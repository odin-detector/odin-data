/*
 *  Created on: 22 Nov 2021
 *      Author: Gary Yendell
 */
#ifndef PARAMETERPUBLISHPLUGIN_H
#define PARAMETERPUBLISHPLUGIN_H

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "IpcChannel.h"

#include <unordered_set>
/**
 * This plugin class looks for the configured parameters on a Frame and publishes them over ZMQ
 */
namespace FrameProcessor {
class ParameterPublishPlugin : public FrameProcessorPlugin {
public:
    ParameterPublishPlugin();
    ~ParameterPublishPlugin() override;
    void process_frame(boost::shared_ptr<Frame> frame) override;
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) override;
    void requestConfiguration(OdinData::IpcMessage& reply) override;
    int get_version_major() override;
    int get_version_minor() override;
    int get_version_patch() override;
    std::string get_version_short() override;
    std::string get_version_long() override;

    // Config message keys
    static const std::string CONFIG_ENDPOINT;
    static const std::string CONFIG_ADD_PARAMETER;
    // Data message keys
    static const std::string DATA_FRAME_NUMBER;
    static const std::string DATA_PARAMETERS;

private:
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Mutex used to make this class thread safe */
    std::mutex mutex_;
    /** Parameters to publish */
    std::unordered_set<std::string> parameters_;
    /** Configured endpoint messages are published on */
    std::string channel_endpoint_;
    /** IpcChannel for publishing messages */
    OdinData::IpcChannel publish_channel_;
    void setup_publish_channel(std::string&& endpoint);
};
} /* namespace FrameProcessor */
#endif // PARAMETERPUBLISHPLUGIN_H
