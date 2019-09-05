#include "ControlUtility.h"
#include "PropertyTreeUtility.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>
#include "zmq/zmq.hpp"

namespace FrameSimulatorTest {

    /** ControlUtility constructor
     *
     * @param ptree - boost property_tree; parsed from initial ini file
     * @param positional_arg - (possible) positional argument
     * @param process_entry - property tree entry accessor for process path
     * @param process_args_entry - property tree entry accessor for process arguments
     * @param process_pid - pid of process
     * @param logger - pointer to logger
     */
    ControlUtility::ControlUtility(boost::property_tree::ptree &ptree,
                                   const std::string &positional_arg,
                                   const std::string &process_entry,
                                   const std::string &process_args_entry,
                                   const std::string &socket_entry,
                                   const std::string &kill_entry,
                                   LoggerPtr &logger) : logger_(logger) {

      // Read process arguments from property tree into std::vector<std::string> command_args_
      PropertyTreeUtility::ini_to_command_args(ptree, process_args_entry, command_args_);

      // Get path to process to run
      process_path_ = ptree.get<std::string>(process_entry);
      PropertyTreeUtility::expandEnvVars(process_path_);

      boost::filesystem::path path(process_path_);

      // Prepend list of process (command) arguments with (<process> (deduced from path) and) (optionally) <positional_arg>

      if (!positional_arg.empty())
        command_args_.insert(command_args_.begin(), positional_arg);
      process_args_ = command_args_;
      process_args_.insert(process_args_.begin(), path.filename().c_str());

      kill_message_ = ptree.get_optional<std::string>(kill_entry);
      socket_ = ptree.get_optional<std::string>(socket_entry);

    }

    /** Run the process
     *
     * @param wait_child - parent to wait for child process to finish
     */
    void ControlUtility::run_process(const bool &wait_child) {

      process_pid_ = fork();

      if (process_pid_ > 0) {
        LOG4CXX_DEBUG(logger_, "Launching " + process_path_ + "(" +
                               boost::lexical_cast<std::string>(process_pid_) + ")");
        int status;
        if (wait_child) {
          wait(NULL);
        }
      }

      if (process_pid_ == 0) {

        std::vector<char *> args;
        for (int s = 0; s < process_args_.size(); s++) {
          args.push_back((char *) process_args_.at(s).data());
        }
        args.push_back(NULL);

        execv(process_path_.c_str(), args.data());

      }

    }

    /** Run the command
     *
     */
    void ControlUtility::run_command() {

      std::string command = process_path_;

      command += " ";
      for (int s = 0; s < command_args_.size(); s++) {
        if (command_args_[s].find("--") != std::string::npos)
          command += command_args_[s] + "=";
        else
          command += command_args_[s] + " ";
        }

      std::system((command + "&").c_str());

    }

    /** End the process
     *
     */
    void ControlUtility::end() {

      if(!kill_message_) {
        LOG4CXX_DEBUG(logger_, "Terminating " + process_path_ + "(" +
                               boost::lexical_cast<std::string>(process_pid_) + ")");
        kill(process_pid_, SIGTERM);
        return;
      }

      zmq::context_t context (1);
      zmq::socket_t socket (context, ZMQ_DEALER);

      socket.connect (socket_.get().c_str());

      LOG4CXX_DEBUG(logger_, "Requesting termination " + process_path_ + "(" +
                               boost::lexical_cast<std::string>(process_pid_) + ")");

      std::string command(kill_message_.get().c_str());
      zmq::message_t request (command.size());
      memcpy (request.data (), command.c_str(), command.size());
      socket.send (request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv (&reply);
      std::string replyMessage(static_cast<char*>(reply.data()), reply.size());

    }
}
