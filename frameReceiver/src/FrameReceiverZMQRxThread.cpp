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
}

FrameReceiverZMQRxThread::~FrameReceiverZMQRxThread()
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Waiting for RX thread to stop....");
    LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread stopped....");

}

void FrameReceiverZMQRxThread::run_specific_service(void)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Running ZMQ RX thread service");

    for (std::vector<uint16_t>::iterator rx_port_itr = config_.rx_ports_.begin(); rx_port_itr != config_.rx_ports_.end(); rx_port_itr++)
    {
        uint16_t rx_port = *rx_port_itr;

        std::stringstream ss;
        ss << "tcp://127.0.0.1:" << rx_port;
        skt_channel_.connect(ss.str().c_str());

        // Register the IPC channel with the reactor
        reactor_.register_channel(skt_channel_, boost::bind(&FrameReceiverZMQRxThread::handle_receive_socket, this));

    }
}

void FrameReceiverZMQRxThread::handle_receive_socket()
{
    // Receive a message from the main thread channel
    std::string rx_msg = skt_channel_.recv(0);

	char *buffer_ptr_ = (char *)frame_decoder_->get_next_payload_buffer();

	// Copy the message into the buffer
	int msg_len = rx_msg.length();
	strncpy(buffer_ptr_, rx_msg.c_str(), msg_len);

	LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << msg_len << " bytes on IPC channel, payload buffer address "
			<< frame_decoder_->get_next_payload_buffer());

	FrameDecoder::FrameReceiveState frame_receive_state = frame_decoder_->process_packet(msg_len);

	// Now check for end of messsage
	if (skt_channel_.eom()){
		// Message complete, notify the decoder
		frame_decoder_->frame_meta_data(1);
	}
}

