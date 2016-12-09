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
        SharedBufferManagerPtr buffer_manager, FrameDecoderPtr frame_decoder, unsigned int tick_period_ms) :
   config_(config),
   logger_(logger),
   buffer_manager_(buffer_manager),
   frame_decoder_(frame_decoder),
   tick_period_ms_(tick_period_ms),
   rx_channel_(ZMQ_PAIR),
   recv_socket_(0),
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
            rx_thread_.join();
            throw FrameReceiverException(thread_init_msg_);
            break;
        }
    }
}

FrameReceiverRxThread::~FrameReceiverRxThread()
{
    run_thread_ = false;
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Waiting for RX thread to stop....");
    rx_thread_.join();
    LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread stopped....");

}

void FrameReceiverRxThread::run_service(void)
{
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Running RX thread service");

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

    // Add the RX channel to the reactor
    reactor_.register_channel(rx_channel_, boost::bind(&FrameReceiverRxThread::handle_rx_channel, this));

    for (std::vector<uint16_t>::iterator rx_port_itr = config_.rx_ports_.begin(); rx_port_itr != config_.rx_ports_.end(); rx_port_itr++)
    {

        uint16_t rx_port = *rx_port_itr;

        // Create the receive socket
        int recv_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (recv_socket < 0)
        {
            std::stringstream ss;
            ss << "RX channel failed to create receive socket for port " << rx_port << " : " << strerror(errno);
            thread_init_msg_ = ss.str();
            thread_init_error_ = true;
            return;
        }

        // Set the socket receive buffer size
        if (setsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &config_.rx_recv_buffer_size_, sizeof(config_.rx_recv_buffer_size_)) < 0)
        {
            std::stringstream ss;
            ss << "RX channel failed to set receive socket buffer size for port " << rx_port << " : " << strerror(errno);
            thread_init_msg_ = ss.str();
            thread_init_error_ = true;
            return;
        }

        // Read it back and display
        int buffer_size;
        socklen_t len = sizeof(buffer_size);
        getsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, &len);
        LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread receive buffer size for port " << rx_port << " is " << buffer_size);

        // Bind the socket to the specified port
        struct sockaddr_in recv_addr;
        memset(&recv_addr, 0, sizeof(recv_addr));

        recv_addr.sin_family      = AF_INET;
        recv_addr.sin_port        = htons(rx_port);
        recv_addr.sin_addr.s_addr = inet_addr(config_.rx_address_.c_str());

        if (recv_addr.sin_addr.s_addr == INADDR_NONE)
        {
            std::stringstream ss;
             ss <<  "Illegal receive address specified: " << config_.rx_address_;
             thread_init_msg_ = ss.str();
             thread_init_error_ = true;
             return;
        }

        if (bind(recv_socket, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1)
        {
            std::stringstream ss;
            ss <<  "RX channel failed to bind receive socket for address " << config_.rx_address_ << " port " << rx_port << " : " << strerror(errno);
            thread_init_msg_ = ss.str();
            thread_init_error_ = true;
            return;
        }

        if (thread_init_error_) break;

        // Add the receive socket to the reactor
        reactor_.register_socket(recv_socket, boost::bind(&FrameReceiverRxThread::handle_receive_socket, this, recv_socket, (int)rx_port));

        recv_sockets_.push_back(recv_socket);
    }

    // Add the tick timer to the reactor
    int tick_timer_id = reactor_.register_timer(tick_period_ms_, 0, boost::bind(&FrameReceiverRxThread::tick_timer, this));

    // Add the buffer monitor timer to the reactor
    int buffer_monitor_timer_id = reactor_.register_timer(3000, 0, boost::bind(&FrameReceiverRxThread::buffer_monitor_timer, this));

    // Register the frame release callback with the decoder
    frame_decoder_->register_frame_ready_callback(boost::bind(&FrameReceiverRxThread::frame_ready, this, _1, _2));

    // Set thread state to running, allows constructor to return
    thread_running_ = true;

    // Run the reactor event loop
    reactor_.run();

    // Cleanup - remove channels, sockets and timers from the reactor and close the receive socket
    reactor_.remove_channel(rx_channel_);
    reactor_.remove_timer(tick_timer_id);
    reactor_.remove_timer(buffer_monitor_timer_id);

    for (std::vector<int>::iterator recv_sock_it = recv_sockets_.begin(); recv_sock_it != recv_sockets_.end(); recv_sock_it++)
    {
        reactor_.remove_socket(*recv_sock_it);
        close(*recv_sock_it);
    }
    recv_sockets_.clear();

    LOG4CXX_DEBUG_LEVEL(1, logger_, "Terminating RX thread service");

}

void FrameReceiverRxThread::handle_rx_channel(void)
{
    // Receive a message from the main thread channel
    std::string rx_msg_encoded = rx_channel_.recv();

    // Parse and handle the message
    try {

        IpcMessage rx_msg(rx_msg_encoded.c_str());

		if ((rx_msg.get_msg_type() == IpcMessage::MsgTypeNotify) &&
			(rx_msg.get_msg_val()  == IpcMessage::MsgValNotifyFrameRelease))
		{

			int buffer_id = rx_msg.get_param<int>("buffer_id", -1);

			if (buffer_id != -1)
			{
				frame_decoder_->push_empty_buffer(buffer_id);
				LOG4CXX_DEBUG_LEVEL(3, logger_, "Added empty buffer ID " << buffer_id << " to queue, length is now "
						<< frame_decoder_->get_num_empty_buffers());
			}
			else
			{
				LOG4CXX_ERROR(logger_, "RX thread received empty frame notification with buffer ID");
			}

		}
		else if ((rx_msg.get_msg_type() == IpcMessage::MsgTypeCmd) &&
				(rx_msg.get_msg_val()  == IpcMessage::MsgValCmdStatus))
		{
		    IpcMessage rx_reply;

			rx_reply.set_msg_type(IpcMessage::MsgTypeAck);
			rx_reply.set_msg_val(IpcMessage::MsgValCmdStatus);
			rx_reply.set_param("count", rx_msg.get_param<int>("count", -1));

		    rx_channel_.send(rx_reply.encode());
		}
		else
		{
			LOG4CXX_ERROR(logger_, "RX thread got unexpected message: " << rx_msg_encoded);

		    IpcMessage rx_reply;

			rx_reply.set_msg_type(IpcMessage::MsgTypeNack);
			rx_reply.set_msg_val(rx_msg.get_msg_val());
			//TODO add error in params

			rx_channel_.send(rx_reply.encode());
		}
    }
    catch (IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
    }

}

void FrameReceiverRxThread::handle_receive_socket(int recv_socket, int recv_port)
{

	if (frame_decoder_->requires_header_peek())
	{
		size_t header_size = frame_decoder_->get_packet_header_size();
		void*  header_buffer = frame_decoder_->get_packet_header_buffer();
		struct sockaddr_in from_addr;
		socklen_t from_len = sizeof(from_addr);
		size_t bytes_received = recvfrom(recv_socket, header_buffer, header_size, MSG_PEEK, (struct sockaddr*)&from_addr, &from_len);
		LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << bytes_received << " header bytes on recv socket");
		frame_decoder_->process_packet_header(bytes_received, recv_port, &from_addr);
	}

	struct iovec io_vec[2];
	io_vec[0].iov_base = frame_decoder_->get_packet_header_buffer();
	io_vec[0].iov_len  = frame_decoder_->get_packet_header_size();
	io_vec[1].iov_base = frame_decoder_->get_next_payload_buffer();
	io_vec[1].iov_len  = frame_decoder_->get_next_payload_size();

	struct msghdr msg_hdr;
	memset((void*)&msg_hdr,  0, sizeof(struct msghdr));
	msg_hdr.msg_name = 0;
	msg_hdr.msg_namelen = 0;
	msg_hdr.msg_iov = io_vec;
	msg_hdr.msg_iovlen = 2;

	size_t bytes_received = recvmsg(recv_socket, &msg_hdr, 0);
	LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << bytes_received << " header/payload bytes on recv socket");

	FrameDecoder::FrameReceiveState frame_receive_state = frame_decoder_->process_packet(bytes_received);
}

void FrameReceiverRxThread::tick_timer(void)
{
	//LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread tick timer fired");
	if (!run_thread_)
	{
		LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread terminate detected in timer");
		reactor_.stop();
	}
}

void FrameReceiverRxThread::buffer_monitor_timer(void)
{
    frame_decoder_->monitor_buffers();
}

void FrameReceiverRxThread::frame_ready(int buffer_id, int frame_number)
{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Releasing frame " << frame_number << " in buffer " << buffer_id);

    IpcMessage ready_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameReady);
    ready_msg.set_param("frame", frame_number);
    ready_msg.set_param("buffer_id", buffer_id);

    rx_channel_.send(ready_msg.encode());

}
