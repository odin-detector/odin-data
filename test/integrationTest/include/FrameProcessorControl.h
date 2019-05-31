#ifndef ODINDATA_FRAMEPROCESSORCONTROL_H
#define ODINDATA_FRAMEPROCESSORCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameProcessorControl {

    public:

        static void launch_processor(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger);

    };

}

#endif //ODINDATA_FRAMEPROCESSORCONTROL_H
