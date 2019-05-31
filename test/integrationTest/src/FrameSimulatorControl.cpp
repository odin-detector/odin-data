#include "FrameSimulatorControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    void FrameSimulatorControl::launch_simulator(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger) {

      std::string detector = ptree.get<std::string>("Main.detector");

      std::vector<std::string> command_args;
      PropertyTreeUtility::ini_to_command_args(ptree, "Simulator-args", command_args);

      std::string process_path = ptree.get<std::string>("Main.simulator");

      command_args.insert(command_args.begin(), detector);
      command_args.insert(command_args.begin(), "frameSimulator");

      ControlUtility::launch_process(process_path, command_args, pid, logger, true);

    }

}