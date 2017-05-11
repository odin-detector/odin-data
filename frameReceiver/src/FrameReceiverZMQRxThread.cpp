/*!
 * FrameReceiverZMQRxThread.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverZMQRxThread.h"
#include <unistd.h>

using namespace FrameReceiver;

FrameReceiverZMQRxThread::FrameReceiverZMQRxThread(FrameReceiverConfig& config, LoggerPtr& logger,
        SharedBufferManagerPtr buffer_manager, FrameDecoderPtr frame_decoder, unsigned int tick_period_ms) :
        		FrameReceiverRxThread(config, logger, buffer_manager, frame_decoder, tick_period_ms),
				logger_(logger),
				skt_channel_(ZMQ_PULL)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "FrameReceiverZMQRxThread constructor entered....");

    // Store the frame decoder as a UDP type frame decoder
    frame_decoder_ = boost::dynamic_pointer_cast<FrameDecoderZMQ>(frame_decoder);
}

FrameReceiverZMQRxThread::~FrameReceiverZMQRxThread()
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Destroying FrameReceiverZMQRxThread....");
}

void FrameReceiverZMQRxThread::run_specific_service(void)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Running ZMQ RX thread service");

    for (std::vector<uint16_t>::iterator rx_port_itr = config_.rx_ports_.begin(); rx_port_itr != config_.rx_ports_.end(); rx_port_itr++)
    {
        uint16_t rx_port = *rx_port_itr;

        std::stringstream ss;
        ss << "tcp://" << config_.rx_address_ << ":" << rx_port;
        skt_channel_.connect(ss.str().c_str());

        // Register the IPC channel with the reactor
        reactor_.register_channel(skt_channel_, boost::bind(&FrameReceiverZMQRxThread::handle_receive_socket, this));

    }
}

void FrameReceiverZMQRxThread::cleanup_specific_service(void)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Cleaning up ZMQ RX thread service");

    // Remove the IPC channel from the reactor
    reactor_.remove_channel(skt_channel_);
    skt_channel_.close();
}

void FrameReceiverZMQRxThread::handle_receive_socket()
{
    // Receive a message from the main thread channel
    std::string rx_msg = skt_channel_.recv();

	char *buffer_ptr_ = (char *)frame_decoder_->get_next_message_buffer();

	// Copy the message into the buffer
	int msg_len = rx_msg.length();
	memcpy(buffer_ptr_, rx_msg.c_str(), msg_len);

	LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << msg_len << " bytes on IPC channel, payload buffer address "
			<< frame_decoder_->get_next_message_buffer());

	FrameDecoder::FrameReceiveState frame_receive_state = frame_decoder_->process_message(msg_len);

	// Now check for end of messsage
	if (skt_channel_.eom()){
		// Message complete, notify the decoder
		frame_decoder_->frame_meta_data(1);
	}
}
