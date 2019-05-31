#include "FrameProcessorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    void FrameProcessorControl::launch_processor(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger) {

      std::vector<std::string> command_args;

      PropertyTreeUtility::ini_to_command_args(ptree, "Processor-args", command_args);

      std::string process_path = ptree.get<std::string>("Main.processor");

      command_args.insert(command_args.begin(), "frameProcessor");

      ControlUtility::launch_process(process_path, command_args, pid, logger);

    }

}