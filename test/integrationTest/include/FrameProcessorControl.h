#ifndef ODINDATA_FRAMEPROCESSORCONTROL_H
#define ODINDATA_FRAMEPROCESSORCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

#include "ControlUtility.h"

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameProcessorControl : public ControlUtility {

    public:

        FrameProcessorControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger);

    };

}

#endif //ODINDATA_FRAMEPROCESSORCONTROL_H
