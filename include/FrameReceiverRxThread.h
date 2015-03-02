/*!
 * FrameReceiverRxThread.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERRXTHREAD_H_
#define FRAMERECEIVERRXTHREAD_H_

#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <queue>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoder.h"

#include "FrameReceiverConfig.h"
#include "FrameReceiverException.h"

namespace FrameReceiver
{

    class FrameReceiverRxThreadException : public FrameReceiverException
    {
    public:
        FrameReceiverRxThreadException(const std::string what) : FrameReceiverException(what) { };
    };

    class FrameReceiverRxThread
    {
    public:
        FrameReceiverRxThread(FrameReceiverConfig& config, LoggerPtr& logger,
                SharedBufferManagerPtr buffer_manager, FrameDecoderPtr frame_decoder,
                unsigned int tick_period_ms=100);
        virtual ~FrameReceiverRxThread();

        void start();
        void stop();


    private:

        void run_service(void);

        void handle_rx_channel(void);
        void handle_receive_socket(void);
        void tick_timer(void);

        FrameReceiverConfig&   config_;
        LoggerPtr              logger_;
        SharedBufferManagerPtr buffer_manager_;
        FrameDecoderPtr        frame_decoder_;
        unsigned int           tick_period_ms_;

        IpcChannel             rx_channel_;
        int                    recv_socket_;
        IpcReactor             reactor_;

        bool                   run_thread_;
        bool                   thread_running_;
        bool                   thread_init_error_;
        boost::thread          rx_thread_;
        std::string            thread_init_msg_;

        std::queue<int>        empty_buffer_queue_;


    };

} // namespace FrameReceiver

#endif /* FRAMERECEIVERRXTHREAD_H_ */
