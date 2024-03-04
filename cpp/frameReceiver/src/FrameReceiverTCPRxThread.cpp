/*!
 * FrameReceiverTCPRxThread.cpp
 *
 */

#include <unistd.h>

#include "FrameReceiverTCPRxThread.h"

using namespace FrameReceiver;

FrameReceiverTCPRxThread::FrameReceiverTCPRxThread(FrameReceiverConfig& config,
                                                   SharedBufferManagerPtr buffer_manager,
                                                   FrameDecoderPtr frame_decoder,
                                                   unsigned int tick_period_ms) :
    FrameReceiverRxThread(config, buffer_manager, frame_decoder, tick_period_ms),
    logger_(log4cxx::Logger::getLogger("FR.TCPRxThread"))
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "FrameReceiverTCPRxThread constructor entered....");

  // Store the frame decoder as a TCP type frame decoder
  frame_decoder_ = boost::dynamic_pointer_cast<FrameDecoderTCP>(frame_decoder);
}

FrameReceiverTCPRxThread::~FrameReceiverTCPRxThread()
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Destroying FrameReceiverTCPRxThread....");
}

void FrameReceiverTCPRxThread::run_specific_service(void)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Running TCP RX thread service");

  for (std::vector<uint16_t>::iterator rx_port_itr = config_.rx_ports_.begin(); rx_port_itr != config_.rx_ports_.end(); rx_port_itr++)
  {

    uint16_t rx_port = *rx_port_itr;

    // Create the receive socket
    int recv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (recv_socket < 0)
    {
      std::stringstream ss;
      ss << "RX channel failed to create receive socket for port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Set the socket receive buffer size
    if (setsockopt(recv_socket, SOL_SOCKET, SO_REUSEADDR, &config_.rx_recv_buffer_size_, sizeof(config_.rx_recv_buffer_size_)) < 0)
    {
      std::stringstream ss;
      ss << "RX channel failed to set receive socket buffer size for port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Read it back and display
    int buffer_size;
    socklen_t len = sizeof(buffer_size);
    getsockopt(recv_socket, SOL_SOCKET, SO_REUSEADDR, &buffer_size, &len);
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
      this->set_thread_init_error(ss.str());
      return;
    }

    if (connect(recv_socket, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1)
    {
      std::stringstream ss;
      ss <<  "RX channel failed to bind receive socket for address " << config_.rx_address_ << " port " << rx_port << " : " << strerror(errno);
      this->set_thread_init_error(ss.str());
      return;
    }

    // Register this socket
    this->register_socket(recv_socket, boost::bind(&FrameReceiverTCPRxThread::handle_receive_socket, this, recv_socket, (int)rx_port));
  }
}

void FrameReceiverTCPRxThread::cleanup_specific_service(void)
{
}

void FrameReceiverTCPRxThread::handle_receive_socket(int recv_socket, int recv_port)
{
  // Receive a message from the main thread channel and place it directly into the
  // provided memory buffer
  void* frame_buffer = frame_decoder_->get_next_message_buffer();
  size_t message_size = frame_decoder_->get_next_message_size();
  size_t bytes_received = 0;

  while (bytes_received < message_size) {
    int msg_len = read(recv_socket, frame_buffer, message_size - bytes_received);
    bytes_received += msg_len;
    frame_buffer += msg_len;
  }
  frame_decoder_->process_message(bytes_received);
}
