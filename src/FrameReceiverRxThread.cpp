/*!
 * FrameReceiverRxThread.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverRxThread.h"
#include <unistd.h>

using namespace FrameReceiver;

FrameReceiverRxThread::FrameReceiverRxThread(FrameReceiverConfig& config, LoggerPtr& logger) :
   config_(config),
   logger_(logger),
   rx_channel_(ZMQ_PAIR)
{

    // Connect the message channel to the main thread
    rx_channel_.connect("inproc://rx_channel");
}

FrameReceiverRxThread::~FrameReceiverRxThread()
{
}

void FrameReceiverRxThread::start()
{
    LOG4CXX_DEBUG(logger_, "Starting RX thread.... " << config_.max_buffer_mem_);
    run_thread_ = true;
    rx_thread_ = boost::thread(&FrameReceiverRxThread::run_service, this);
}

void FrameReceiverRxThread::stop()
{
    run_thread_ = false;
    LOG4CXX_DEBUG(logger_, "Waiting for RX thread to stop....");
    rx_thread_.join();
    LOG4CXX_DEBUG(logger_, "RX thread stopped....");
}

void FrameReceiverRxThread::run_service(void)
{
    LOG4CXX_DEBUG(logger_, "Running RX thread service");

    while (run_thread_)
    {
        if (rx_channel_.poll(10))
        {
            std::string msg = rx_channel_.recv();
            LOG4CXX_DEBUG(logger_, "RX Thread got message: " << msg);

            rx_channel_.send(msg);
        }
    }
    LOG4CXX_DEBUG(logger_, "Terminating RX thread service");

}
