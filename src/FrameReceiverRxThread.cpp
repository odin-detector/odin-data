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
   rx_channel_(ZMQ_PAIR),
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

    // Set thread state to running, allows constructor to return
    thread_running_ = true;

    // Enter event loop
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
