#include "FrameReceiverControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    /** FrameReceiverControl constructor
     *
     * @param ptree - boost property_tree; parsed from initial ini file
     * @param process_pid - pid of process
     * @param logger - pointer to logger
     */
    FrameReceiverControl::FrameReceiverControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, "", "Main.receiver", "Receiver-args", process_pid, logger) {

    }

}
