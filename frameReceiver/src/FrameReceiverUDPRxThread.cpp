/*!
 * FrameReceiverUDPRxThread.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <unistd.h>

#include "FrameReceiverUDPRxThread.h"

using namespace FrameReceiver;

FrameReceiverUDPRxThread::FrameReceiverUDPRxThread(FrameReceiverConfig& config,
                                                   SharedBufferManagerPtr buffer_manager,
                                                   FrameDecoderPtr frame_decoder,
                                                   unsigned int tick_period_ms) :
    FrameReceiverRxThread(config, buffer_manager, frame_decoder, tick_period_ms),
    logger_(log4cxx::Logger::getLogger("FR.UDPRxThread"))
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "FrameReceiverUDPRxThread constructor entered....");

  // Store the frame decoder as a UDP type frame decoder
  frame_decoder_ = boost::dynamic_pointer_cast<FrameDecoderUDP>(frame_decoder);
}

FrameReceiverUDPRxThread::~FrameReceiverUDPRxThread()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Destroying FrameReceiverUDPRxThread....");
}

void FrameReceiverUDPRxThread::run_specific_service(void)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running UDP RX thread service");

  for (std::vector<uint16_t>::iterator rx_port_itr = config_.rx_ports_.begin(); rx_port_itr != config_.rx_ports_.end(); rx_port_itr++)
  {

    uint16_t rx_port = *rx_port_itr;

    // Create the receive socket
    int recv_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recv_socket < 0)
    {
      std::stringstream ss;
      ss << "RX channel failed to create receive socket for port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Set the socket receive buffer size
    if (setsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &config_.rx_recv_buffer_size_, sizeof(config_.rx_recv_buffer_size_)) < 0)
    {
      std::stringstream ss;
      ss << "RX channel failed to set receive socket buffer size for port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Read it back and display
    int buffer_size;
    socklen_t len = sizeof(buffer_size);
    getsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &buffer_size, &len);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "RX thread receive buffer size for port " << rx_port << " is " << buffer_size / 2);

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
      this->set_thread_init_error(ss.str());
      return;
    }

    if (bind(recv_socket, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1)
    {
      std::stringstream ss;
      ss <<  "RX channel failed to bind receive socket for address " << config_.rx_address_ << " port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Register this socket
    this->register_socket(recv_socket, boost::bind(&FrameReceiverUDPRxThread::handle_receive_socket, this, recv_socket, (int)rx_port));
  }
}

void FrameReceiverUDPRxThread::cleanup_specific_service(void)
{
}

void FrameReceiverUDPRxThread::handle_receive_socket(int recv_socket, int recv_port)
{

  struct iovec io_vec[2];
  uint8_t iovec_entry = 0;

  struct sockaddr_in from_addr;

  if (frame_decoder_->requires_header_peek())
  {
    size_t header_size = frame_decoder_->get_packet_header_size();
    void*  header_buffer = frame_decoder_->get_packet_header_buffer();
    socklen_t from_len = sizeof(from_addr);
    size_t bytes_received = recvfrom(recv_socket, header_buffer, header_size, MSG_PEEK, (struct sockaddr*)&from_addr, &from_len);
    LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << bytes_received << " header bytes on recv socket");
    frame_decoder_->process_packet_header(bytes_received, recv_port, &from_addr);

    io_vec[iovec_entry].iov_base = frame_decoder_->get_packet_header_buffer();
    io_vec[iovec_entry].iov_len  = frame_decoder_->get_packet_header_size();
    iovec_entry++;
  }

  io_vec[iovec_entry].iov_base = frame_decoder_->get_next_payload_buffer();
  io_vec[iovec_entry].iov_len  = frame_decoder_->get_next_payload_size();
  iovec_entry++;

  struct msghdr msg_hdr;
  memset((void*)&msg_hdr,  0, sizeof(struct msghdr));
  msg_hdr.msg_name = (void*)&from_addr;
  msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
  msg_hdr.msg_iov = io_vec;
  msg_hdr.msg_iovlen = iovec_entry;

  size_t bytes_received = recvmsg(recv_socket, &msg_hdr, 0);
  LOG4CXX_DEBUG_LEVEL(3, logger_, "RX thread received " << bytes_received << " header/payload bytes on recv socket, "
      "payload buffer address " << frame_decoder_->get_next_payload_buffer());

  FrameDecoder::FrameReceiveState frame_receive_state = frame_decoder_->process_packet(bytes_received, recv_port, &from_addr);
}
