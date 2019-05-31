#ifndef ODINDATA_CONTROLUTILITY_H
#define ODINDATA_CONTROLUTILITY_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

using namespace log4cxx;

namespace FrameSimulatorTest {

    class ControlUtility {

    public:

        static void launch_process(const std::string &process_path,
                                   std::vector<std::string>& args,
                                   pid_t &process_pid,
                                   LoggerPtr &logger,
                                   const bool &wait_child = false);
        
    };

}

#endif //ODINDATA_CONTROLUTILITY_H
