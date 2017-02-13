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

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoder.h"

#include "FrameReceiverConfig.h"
#include "OdinDataException.h"

using namespace OdinData;

namespace FrameReceiver
{

    class FrameReceiverRxThreadException : public OdinData::OdinDataException
    {
    public:
        FrameReceiverRxThreadException(const std::string what) : OdinData::OdinDataException(what) { };
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

        void frame_ready(int buffer_id, int frame_number);

    private:

        void run_service(void);

        void handle_rx_channel(void);
        void handle_receive_socket(int socket_fd, int recv_port);
        void tick_timer(void);
        void buffer_monitor_timer(void);

        FrameReceiverConfig&   config_;
        LoggerPtr              logger_;
        SharedBufferManagerPtr buffer_manager_;
        FrameDecoderPtr        frame_decoder_;
        unsigned int           tick_period_ms_;

        IpcChannel             rx_channel_;
        int                    recv_socket_;
        std::vector<int>       recv_sockets_;
        IpcReactor             reactor_;

        bool                   run_thread_;
        bool                   thread_running_;
        bool                   thread_init_error_;
        boost::thread          rx_thread_;
        std::string            thread_init_msg_;

    };

} // namespace FrameReceiver

#endif /* FRAMERECEIVERRXTHREAD_H_ */
