#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>

namespace FrameSimulatorTest {

     ControlUtility::ControlUtility(boost::property_tree::ptree &ptree,
                                    const std::string &command,
                                    const std::string &process_entry,
                                    const std::string &process_args_entry,
                                    pid_t &process_pid,
                                    LoggerPtr &logger) : process_pid_(process_pid), logger_(logger) {

       PropertyTreeUtility::ini_to_command_args(ptree, process_args_entry, process_args_);

       process_path_ = ptree.get<std::string>(process_entry);

       boost::filesystem::path path(process_path_);

       process_args_.insert(process_args_.begin(), command);
       process_args_.insert(process_args_.begin(), path.filename().c_str());

     }

     void ControlUtility::run_process(const bool &wait_child) {

       process_pid_ = fork();
       
        if (process_pid_ > 0) {
        LOG4CXX_DEBUG(logger_, "Launching " + process_path_ + "(" +
                              boost::lexical_cast<std::string>(process_pid_) + ")");
        if (wait_child) {
          wait(NULL);
        }
      }

      if (process_pid_ == 0) {

        std::vector<char*> args;
        for (int s =0; s < process_args_.size(); s++) {
          args.push_back((char*)process_args_.at(s).data());
        }
        args.push_back(NULL);
      execv(process_path_.c_str(), args.data());


     }

    }

}