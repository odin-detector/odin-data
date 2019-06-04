#include "FrameSimulatorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    /** FrameSimulatorControl constructor
     *
     * @param ptree - boost property_tree; parsed from initial ini file
     * @param process_pid - pid of process
     * @param logger - pointer to logger
     */
    FrameSimulatorControl::FrameSimulatorControl(const std::string &detector, boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, detector, "Main.simulator", "Simulator-args", process_pid, logger) {

    }

}
