#include "FrameProcessorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    /** FrameProcessorControl constructor
     *
     * @param ptree - boost property_tree; parsed from initial ini file
     * @param process_pid - pid of process
     * @param logger - pointer to logger
     */
    FrameProcessorControl::FrameProcessorControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger) :
            ControlUtility(ptree, "", "Main.processor", "Processor-args", process_pid, logger) {

    }

}
