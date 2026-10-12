/*
 * OffsetAdjustmentPlugin.h
 *
 *  Created on: 16 Aug 2018
 *      Author: Matt Taylor
 */

#ifndef FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_
#define FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "ClassLoader.h"
#include "FrameProcessorPlugin.h"

#include <atomic>

namespace FrameProcessor {

const int DEFAULT_OFFSET_ADJUSTMENT = 0;
static const std::string OFFSET_ADJUSTMENT_CONFIG = "offset_adjustment";

/**
 * This plugin class alters the frame offset by a configured amount
 *
 */
class OffsetAdjustmentPlugin : public FrameProcessorPlugin {
public:
    OffsetAdjustmentPlugin();
    ~OffsetAdjustmentPlugin() override;
    void process_frame(std::shared_ptr<Frame> frame) override;
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
    /** Offset adjustment to use **/
    std::atomic<int64_t> offset_adjustment_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_OFFSETADJUSTMENTPLUGIN_H_ */
