//
// Created by hir12111 on 30/10/18.
//

#ifndef SUMPLUGIN_H
#define SUMPLUGIN_H

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"

namespace FrameProcessor {

static const std::string SUM_PARAM_NAME = "sum";

/**
 * This plugin class calculates the sum of each pixel and adds it as a parameter
 */
class SumPlugin : public FrameProcessorPlugin {
public:
    SumPlugin();

    ~SumPlugin() override;

    void process_frame(boost::shared_ptr<Frame> frame) override;

    int get_version_major() override;

    int get_version_minor() override;

    int get_version_patch() override;

    std::string get_version_short() override;

    std::string get_version_long() override;

private:
    /** Pointer to logger */
    LoggerPtr logger_;
};

} /* namespace FrameProcessor */

#endif // SUMPLUGIN_H
