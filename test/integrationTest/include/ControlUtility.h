#ifndef ODINDATA_CONTROLUTILITY_H
#define ODINDATA_CONTROLUTILITY_H

#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include <log4cxx/logger.h>
#include <vector>

using namespace log4cxx;

namespace FrameSimulatorTest {

    /**
     * This is a base class for running odin processes (frameReceiver, frameProcessor & frameSimulator)
     *
     */
    class ControlUtility {

    public:

        /** Construct a ControlUtlity to run a process */
        ControlUtility(boost::property_tree::ptree &ptree,
                       const std::string &positional_arg,
                       const std::string &process_entry,
                       const std::string &process_args_entry,
                       const std::string &socket_entry,
                       const std::string &kill_entry,
                       LoggerPtr &logger);

        /** Run the process */
        void run_process(const bool &wait_child = false);

        /** Run command */
        void run_command();

        /** End process */
        void end();

    protected:

        /** Path of process to run */
        std::string process_path_;

        /** Process command arguments */
        std::vector<std::string> process_args_;

        /** Command arguments */
        std::vector<std::string> command_args_;

        /** Socket for messages */
        boost::optional<std::string> socket_;

        /** Kill message */
        boost::optional<std::string> kill_message_;

        /** pid of process */
        pid_t process_pid_;

        /** Pointer to logger */
        LoggerPtr logger_;

    };

}

#endif //ODINDATA_CONTROLUTILITY_H
