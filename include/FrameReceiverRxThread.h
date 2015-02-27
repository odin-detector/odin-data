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

#include "IpcChannel.h"
#include "IpcMessage.h"
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
                unsigned int tick_period_us=1000);
        virtual ~FrameReceiverRxThread();

        void start();
        void stop();


    private:

        void run_service(void);
        void check_deadline(void);

        void start_receive(void);
        void handle_receive(const boost::system::error_code& error, std::size_t bytes_received);

        FrameReceiverConfig&   config_;
        LoggerPtr              logger_;
        SharedBufferManagerPtr buffer_manager_;
        FrameDecoderPtr        frame_decoder_;
        unsigned int           tick_period_us_;

        IpcChannel                  rx_channel_;

        boost::asio::io_service        io_service_;
        boost::asio::ip::udp::endpoint remote_endpoint_;
        boost::asio::ip::udp::socket   recv_socket_;
        boost::asio::deadline_timer    deadline_timer_;

        bool                   run_thread_;
        bool                   thread_running_;
        bool                   thread_init_error_;
        boost::thread          rx_thread_;
        std::string            thread_init_msg_;


    };

} // namespace FrameReceiver

#endif /* FRAMERECEIVERRXTHREAD_H_ */
