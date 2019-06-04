#include "FrameReceiverControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    FrameReceiverControl::FrameReceiverControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, "", "Main.receiver", "Receiver-args", process_pid, logger) {

    }

}