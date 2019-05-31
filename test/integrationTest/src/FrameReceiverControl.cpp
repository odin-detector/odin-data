#include "FrameReceiverControl.h"
#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

namespace FrameSimulatorTest {

    void FrameReceiverControl::launch_receiver(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger) {

      std::vector<std::string> command_args;
      PropertyTreeUtility::ini_to_command_args(ptree, "Receiver-args", command_args);

      std::string process_path = ptree.get<std::string>("Main.receiver");

      command_args.insert(command_args.begin(), "frameReceiver");

      ControlUtility::launch_process(process_path, command_args, pid, logger);

    }

}