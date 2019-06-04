#ifndef ODINDATA_FRAMERECEIVERCONTROL_H
#define ODINDATA_FRAMERECEIVERCONTROL_H

#include <boost/property_tree/ptree.hpp>
#include <log4cxx/logger.h>

#include "ControlUtility.h"

using namespace log4cxx;

namespace FrameSimulatorTest {

    class FrameReceiverControl : public ControlUtility {

    public:

        FrameReceiverControl(boost::property_tree::ptree &ptree, pid_t &process_pid, LoggerPtr &logger);

    };

}
#endif //ODINDATA_FRAMERECEIVERCONTROL_H
