/*!
 * FrameReceiverRxThread.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERRXTHREAD_H_
#define FRAMERECEIVERRXTHREAD_H_

#include <boost/thread.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "IpcChannel.h"

#include "FrameReceiverConfig.h"
#include "FrameReceiverException.h"

namespace FrameReceiver
{

    class FrameReceiverRxThread
    {
    public:
        FrameReceiverRxThread(FrameReceiverConfig& config, LoggerPtr& logger);
        virtual ~FrameReceiverRxThread();

        void start();
        void stop();


    private:

        void run_service(void);

        FrameReceiverConfig& config_;
        LoggerPtr            logger_;
        boost::thread        rx_thread_;
        bool                 run_thread_;
        bool                 thread_running_;
        bool                 thread_init_error_;
        std::string          thread_init_msg_;

        IpcChannel           rx_channel_;
    };

} // namespace FrameReceiver

#endif /* FRAMERECEIVERRXTHREAD_H_ */
