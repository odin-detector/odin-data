#ifndef ODINDATA_FRAMESIMULATORCONTROL_H
#define ODINDATA_FRAMESIMULATORCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

#include "ControlUtility.h"

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameSimulatorControl : public ControlUtility {

    public:

        FrameSimulatorControl(const std::string &detector, boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger);

    };

}

#endif //ODINDATA_FRAMESIMULATORCONTROL_H
