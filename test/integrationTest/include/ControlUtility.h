#ifndef ODINDATA_CONTROLUTILITY_H
#define ODINDATA_CONTROLUTILITY_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>
#include <vector>

using namespace log4cxx;

namespace FrameSimulatorTest {

    class ControlUtility {

    public:

        ControlUtility(boost::property_tree::ptree &ptree,
                       const std::string &command,
                       const std::string &process_entry,
                       const std::string &process_args_entry,
                       pid_t &process_pid,
                       LoggerPtr &logger);

        void run_process(const bool &wait_child = false);

    protected:

        std::string process_path_;
        std::vector<std::string> process_args_;
        pid_t &process_pid_;

        LoggerPtr logger_;

    };

}

#endif //ODINDATA_CONTROLUTILITY_H
