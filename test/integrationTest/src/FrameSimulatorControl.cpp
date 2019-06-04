#include "FrameSimulatorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    FrameSimulatorControl::FrameSimulatorControl(const std::string &detector, boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, detector, "Main.simulator", "Simulator-args", process_pid, logger) {

    }

}