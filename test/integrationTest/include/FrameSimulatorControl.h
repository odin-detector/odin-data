#ifndef ODINDATA_FRAMESIMULATORCONTROL_H
#define ODINDATA_FRAMESIMULATORCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameSimulatorControl {

    public:

        static void launch_simulator(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger);

    };

}

#endif //ODINDATA_FRAMESIMULATORCONTROL_H
