#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

#include <boost/lexical_cast.hpp>
#include <sys/wait.h>

namespace FrameSimulatorTest {

    void ControlUtility::launch_process(const std::string &process_path,
                                        std::vector<std::string>& arguments,
                                        pid_t &process_pid,
                                        LoggerPtr &logger,
                                        const bool &wait_child) {

      int status;

      process_pid = fork();

      if (process_pid > 0) {
        LOG4CXX_DEBUG(logger, "Launching " + process_path + "(" +
                              boost::lexical_cast<std::string>(process_pid) + ")");
        if (wait_child) {
          wait(NULL);
        }
      }

      if (process_pid == 0) {

        std::vector<char*> args;
        for (int s =0; s < arguments.size(); s++) {
          args.push_back((char*)arguments.at(s).data());
        }
        args.push_back(NULL);

        execv(process_path.c_str(), args.data());

      }

    }

}