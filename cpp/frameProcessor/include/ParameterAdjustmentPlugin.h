/*
 * ParameterAdjustmentPlugin.h
 *
 *  Created on: 6 Aug 2018
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_
#define FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "ClassLoader.h"
#include "FrameProcessorPlugin.h"

namespace FrameProcessor {
static const std::string PARAMETER_NAME_CONFIG = "parameter";
static const std::string PARAMETER_INPUT_CONFIG = "input";
static const std::string PARAMETER_ADJUSTMENT_CONFIG = "adjustment";

/**
 * This plugin class alters parameters named in a list by a configured amount added on
 * to the frame number. The parameter will be added to the frame if it doesn't already exist
 *
 */
class ParameterAdjustmentPlugin : public FrameProcessorPlugin {
public:
    ParameterAdjustmentPlugin();
    ~ParameterAdjustmentPlugin() override;
    void process_frame(boost::shared_ptr<Frame> frame) override;
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) override;
    int get_version_major() override;
    int get_version_minor() override;
    int get_version_patch() override;
    std::string get_version_short() override;
    std::string get_version_long() override;

private:
    void requestConfiguration(OdinData::IpcMessage& reply) override;

    /** Pointer to logger */
    LoggerPtr logger_;
    /** Map of parameter adjustments to use for each parameter **/
    std::map<std::string, int64_t> parameter_adjustments_;
    /** Map of input parameters to use for each parameter **/
    std::map<std::string, std::string> parameter_inputs_;
    std::mutex mutex_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_PARAMETERADJUSTMENTPLUGIN_H_ */
