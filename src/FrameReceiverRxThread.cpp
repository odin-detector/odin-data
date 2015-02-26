/*!
 * FrameReceiverRxThread.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverRxThread.h"
#include <unistd.h>

using namespace FrameReceiver;

FrameReceiverRxThread::FrameReceiverRxThread(FrameReceiverConfig& config, LoggerPtr& logger,
        SharedBufferManagerPtr buffer_manager, FrameDecoderPtr frame_decoder, unsigned int tick_period_us) :
   config_(config),
   logger_(logger),
   buffer_manager_(buffer_manager),
   frame_decoder_(frame_decoder),
   tick_period_us_(tick_period_us),
   rx_channel_(ZMQ_PAIR),
   recv_socket_(io_service_, boost::asio::ip::udp::endpoint(
           boost::asio::ip::address::from_string(config_.rx_address_), config_.rx_port_)),
   deadline_timer_(io_service_),
   run_thread_(true),
   thread_running_(false),
   thread_init_error_(false),
   rx_thread_(boost::bind(&FrameReceiverRxThread::run_service, this))
{
    // Wait for the thread service to initialise and be running properly, so that
    // this constructor only returns once the object is fully initialised (RAII).
    // Monitor the thread error flag and throw an exception if initialisation fails

    while (!thread_running_)
    {
        if (thread_init_error_) {
            throw FrameReceiverException(thread_init_msg_);
            break;
        }
    }
}

FrameReceiverRxThread::~FrameReceiverRxThread()
{
    run_thread_ = false;
    LOG4CXX_DEBUG(logger_, "Waiting for RX thread to stop....");
    rx_thread_.join();
    LOG4CXX_DEBUG(logger_, "RX thread stopped....");

}

void FrameReceiverRxThread::run_service(void)
{
    LOG4CXX_DEBUG(logger_, "Running RX thread service");

    // Connect the message channel to the main thread
    try {
        rx_channel_.connect(config_.rx_channel_endpoint_);
    }
    catch (zmq::error_t& e) {
        std::stringstream ss;
        ss << "RX channel connect to endpoint " << config_.rx_channel_endpoint_ << " failed: " << e.what();
        thread_init_msg_ = ss.str();
        thread_init_error_ = true;
        return;
    }

    // Intialise the deadline timer
    deadline_timer_.expires_from_now(boost::posix_time::microseconds(tick_period_us_));
    check_deadline();

    // Initialise the receiver
    start_receive();

    // Set thread state to running, allows constructor to return
    thread_running_ = true;

    // Enter event loop and run the async IO service until there are no more handlers registered
    while (run_thread_)
    {
        boost::system::error_code io_service_ec;
        io_service_.run(io_service_ec);

    }
    LOG4CXX_DEBUG(logger_, "Terminating RX thread service");

}

void FrameReceiverRxThread::check_deadline(void)
{

    if (deadline_timer_.expires_at() < boost::asio::deadline_timer::traits_type::now())
    {
        recv_socket_.cancel();
    }

    if (run_thread_)
    {
        deadline_timer_.expires_from_now(boost::posix_time::microseconds(tick_period_us_));
        deadline_timer_.async_wait(boost::bind(&FrameReceiverRxThread::check_deadline, this));
    }
}

void FrameReceiverRxThread::start_receive(void)
{

    recv_socket_.async_receive_from(
            boost::asio::buffer(boost::asio::buffer(frame_decoder_->get_packet_header_buffer().get(),
                    frame_decoder_->packet_header_size())),
            remote_endpoint_,
            boost::bind(&FrameReceiverRxThread::handle_receive, this,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
    );
}

void FrameReceiverRxThread::handle_receive(const boost::system::error_code& error_code, std::size_t bytes_received)
{
    static int lastFrameReceived = 0;
    if (!error_code)
    {
        LOG4CXX_DEBUG(logger_, "Received " << bytes_received << " bytes on socket");
    }
    else
    {
        // An error code of operation aborted means that the deadline timer called cancel on this
        // socket operation, i.e. the receive timed out and our event loop can tick
        if (error_code == boost::asio::error::operation_aborted)
        {
            if (run_thread_)
            {
                if (rx_channel_.poll(0))
                {
                    std::string rx_msg_encoded = rx_channel_.recv();
                    //LOG4CXX_DEBUG(logger_, "RX Thread got message: " << rx_msg_encoded);
                    IpcMessage rx_msg(rx_msg_encoded.c_str());
                    IpcMessage rx_reply;

                    if ((rx_msg.get_msg_type() == IpcMessage::MsgTypeNotify) &&
                        (rx_msg.get_msg_val()  == IpcMessage::MsgValNotifyFrameRelease))
                    {

                        int buffer_id = rx_msg.get_param<int>("buffer_id", -1);
                        uint64_t* buffer_addr = reinterpret_cast<uint64_t*>(buffer_manager_->get_buffer_address(buffer_id));
                        buffer_addr[0] = lastFrameReceived;
                        buffer_addr[1] = 0x12345678;
                        buffer_addr[2] = 0xdeadbeef;

                        usleep(100000);

                        // Simulate receiving a frame then sending a ready notification back
                        rx_reply.set_msg_type(IpcMessage::MsgTypeNotify);
                        rx_reply.set_msg_val(IpcMessage::MsgValNotifyFrameReady);
                        rx_reply.set_param("buffer_id", rx_msg.get_param<int>("buffer_id", -1));
                        rx_reply.set_param("frame", lastFrameReceived++);
                    }
                    else if ((rx_msg.get_msg_type() == IpcMessage::MsgTypeCmd) &&
                            (rx_msg.get_msg_val()  == IpcMessage::MsgValCmdStatus))
                    {
                        rx_reply.set_msg_type(IpcMessage::MsgTypeAck);
                        rx_reply.set_msg_val(IpcMessage::MsgValCmdStatus);
                        rx_reply.set_param("count", rx_msg.get_param<int>("count", -1));
                    }
                    else
                    {
                        LOG4CXX_ERROR(logger_, "RX thread got unexpected message: " << rx_msg_encoded);
                        rx_reply.set_msg_type(IpcMessage::MsgTypeNack);
                        rx_reply.set_msg_val(rx_msg.get_msg_val());
                        //TODO add error in params
                    }
                    rx_channel_.send(rx_reply.encode());
                }

                start_receive();
            }
        }
        else
        {
            LOG4CXX_ERROR(logger_, "Got error on asynchronous receive: " << error_code);
        }
    }
}
