#ifndef ODINDATA_FRAMERECEIVERCONTROL_H
#define ODINDATA_FRAMERECEIVERCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameReceiverControl {

    public:

        static void launch_receiver(boost::property_tree::ptree &ptree, pid_t &pid, LoggerPtr &logger);

    };

}

#endif //ODINDATA_FRAMERECEIVERCONTROL_H
