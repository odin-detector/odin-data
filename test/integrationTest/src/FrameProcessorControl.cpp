#include "FrameProcessorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    FrameProcessorControl::FrameProcessorControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, "", "Main.processor", "Processor-args", process_pid, logger) {

    }

}