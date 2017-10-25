/*!
 * IpcChannel.cpp - implementation of interprocess communication channels for odin-data
 *
 *  Created on: Feb 6, 2015
 *      Author: Tim Nicholls, STFC Application Engineering
 */

#include "IpcChannel.h"
#include <stdio.h>

#define within(num) (int) ((float) (num) * random () / (RAND_MAX + 1.0))

using namespace OdinData;

//! Retrieve the single IpcContext instance.
//!
//! This static method retrieves the singleton IpcContext instance used
//! by call IpcChannels in an application.
//!
IpcContext& IpcContext::Instance(void)
{
  static IpcContext ipcContext;
  return ipcContext;
}

//! Retrieve the underlying ZeroMQ context from the IpcContext instance
//!
//! This method is used by IpcChannel instances to access the underlying
//! ZeroMQ context during socket initialisation
//!
zmq::context_t& IpcContext::get(void)
{
  return zmq_context_;
}

//! Constructor for the IpcContext class
//!
//! This private constructor initialises the IpcConext instance. It
//! should not be called directly, rather via the Instance() singleton
//! static method. The underlying IpcContext is initialised with the
//! specified number of IO thread.
//!
//! \param[in] io_threads - number of IO threads to create.
//!
IpcContext::IpcContext(int io_threads) :
    zmq_context_(io_threads)
{
}

//! Construct an IpcChannel object
//!
//! This constructor creates an IpcChannel object, initialising an underlying
//! ZeroMQ socket of the specified type. If the requested type is DEALER,
//! the socket identity is set using a UUID-like syntax.
//!
//! \param[in] type - ZeroMQ socket type
//!
IpcChannel::IpcChannel(int type) :
    context_(IpcContext::Instance()),
    socket_(context_.get(), type),
    socket_type_(type)    
{
  // Set the socket identity for DEALER sockets
  if (type == ZMQ_DEALER)
  {
    char identity[10] = {};
    sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
    socket_.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  }
}

//! Construct an IpcChannel object
//!
//! This constructor creates an IpcChannel object, initialising an underlying
//! ZeroMQ socket of the specified type. If the requested type is DEALER,
//! the socket identity is set using identity string passed as an argument.
//!
//! \param[in] type - ZeroMQ socket type
//! \param[in] identity - identity string to use for DEALER sockets
//!
IpcChannel::IpcChannel(int type, const std::string identity) :
  context_(IpcContext::Instance()),
  socket_(context_.get(), type),
  socket_type_(type)
{
  // Set the socket identity for DEALER sockets to the specified value
  if (type == ZMQ_DEALER) {
    socket_.setsockopt(ZMQ_IDENTITY, identity.c_str(), identity.length());
  }
}

//! IpcChannel destructor
//!
IpcChannel::~IpcChannel()
{
}

//! Bind the IpcChannel to an endpoint
//!
//! This method binds the IpcChannel to the endpoint specified in the
//! argument in ZeroMQ URI syntax. Bound endpoints are stored to allow
//! them to be unbound cleanly if necessary.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::bind(const char* endpoint)
{

  // Bind the socket to the specified endpoint
  socket_.bind(endpoint);

  // ZeroMQ resolves wildcarded endpoints to a complete address. This address
  // MUST be used in any unbind call, so use ZMQ_LAST_ENDPOINT to determine
  // the resolved endpoint and store both in the map.

  char resolved_endpoint[256];
  std::size_t endpoint_size = sizeof(resolved_endpoint);

  socket_.getsockopt(ZMQ_LAST_ENDPOINT, resolved_endpoint, &endpoint_size);
  bound_endpoints_[std::string(endpoint)] = std::string(resolved_endpoint);
}

//! Bind the IpcChannel to an endpoint
//!
//! This method binds the IpcChannel to the endpoint specified in the
//! argument in ZeroMQ URI syntax. Bound endpoints are stored to allow
//! them to be unbound cleanly if necessary.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::bind(std::string& endpoint)
{
  this->bind(endpoint.c_str());
}

//! Unbind the IpcChannel from an endpoint
//!
//! This method unbinds the IpcChannel from the endpoint specified in the
//! argument in ZeroMQ URI syntax.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::unbind(const char *endpoint)
{
  std::string endpoint_str(endpoint);
  if (bound_endpoints_.count(endpoint_str))
  {
    socket_.unbind(bound_endpoints_[endpoint_str].c_str());
    bound_endpoints_.erase(endpoint_str);
  }
}

//! Unbind the IpcChannel from an endpoint
//!
//! This method unbinds the IpcChannel from the endpoint specified in the
//! argument in ZeroMQ URI syntax.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::unbind(const std::string& endpoint)
{
  this->unbind(endpoint.c_str());
}

//! Check if the IpcChannel is bound to an endpoint
//!
//! This method checks if the IpcChannel is currently bound to the
//! endpoint specified in the argument.
//!
//! \param[in] endpoint - endpoint URI
//! \return - boolean indicating if endpoint bound
//!
bool IpcChannel::has_bound_endpoint(const std::string& endpoint)
{
  return (bound_endpoints_.count(endpoint) > 0);
}

//! Connect the IpcChannel to an endpoint
//!
//! This method connects the IpcChannel to an endpoint specified in the
//! argument in ZeroMQ URI syntax.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::connect(const char* endpoint)
{
  socket_.connect(endpoint);
}

//! Connect the IpcChannel to an endpoint
//!
//! This method connects the IpcChannel to an endpoint specified in the
//! argument in ZeroMQ URI syntax.
//!
//! \param[in] endpoint - endpoint URI
//!
void IpcChannel::connect(std::string& endpoint)
{
  this->connect(endpoint.c_str());
}

//! Subscribe the IpcChannel to a topic
//!
//! This method subscribes the IpcChannel to a topic string specified
//! in the argument. This is applicable to ZMQ_SUB type channels used
//! in publish and subscribe patterns.
//!
//! \param[in] topic - topic to subscribe to
//!
void IpcChannel::subscribe(const char* topic)
{
  socket_.setsockopt(ZMQ_SUBSCRIBE, topic, strlen(topic));
}

//! Send a message on the IpcChannel
//!
//! This method sends the specified message on the IpcChannel, copying the
//! message string into a ZeroMQ message buffer first. The optional flags argument
//! specifies any ZeroMQ flags to use (e.g. ZMQ_DONTWAIT, ZMQ_SNDMORE). The optional
//! identity_str argument is used to identify the destination identity when using
//! DEALER-ROUTER channels.
//!
//! \param[in] message_str - message string to send
//! \param[in] flags - ZeroMQ message send flags (default value 0)
//! \param[in] identity_str - identity of the destination endpoint to use for ROUTER sockets
//!
void IpcChannel::send(std::string& message_str, int flags, const std::string& identity_str)
{
  // Set and send the destination identity for ROUTER type channels
  if (socket_type_ == ZMQ_ROUTER) {
    router_send_identity(identity_str);
  }

  // Determine the message size and copy into ZeroMQ message
  size_t msg_size = message_str.size();
  zmq::message_t msg(msg_size);
  memcpy(msg.data(), message_str.data(), msg_size);

  // Send the message on the underlying socket with the requested flags
  socket_.send(msg, flags);
}

//! Send a message on the IpcChannel
//!
//! This method sends the specified message on the IpcChannel, copying the null-terminated
//! message string into a ZeroMQ message buffer first. The optional flags argument
//! specifies any ZeroMQ flags to use (e.g. ZMQ_DONTWAIT, ZMQ_SNDMORE). The optional
//! identity_str argument is used to identify the destination identity when using
//! DEALER-ROUTER channels.
//!
//! \param[in] message_str - pointer to null-terminated message string to send
//! \param[in] flags - ZeroMQ message send flags (default value 0)
//! \param[in] identity_str - identity of the destination endpoint to use for ROUTER sockets
//!
void IpcChannel::send(const char* message, int flags, const std::string& identity_str)
{
  // Set and send the destination identity for ROUTER type channels
  if (socket_type_ == ZMQ_ROUTER) {
    router_send_identity(identity_str);
  }

  // Determine the message size and copy into ZeroMQ message
  size_t msg_size = strlen(message);
  zmq::message_t msg(msg_size);
  memcpy(msg.data(), message, msg_size);

  // Send the message on the underlying socket with the requested flags
  socket_.send(msg, flags);
}

//! Send a message on the IpcChannel
//!
//! This method sends the specified message on the IpcChannel, copying the message from
//! the specified buffer into a ZeroMQ message buffer first. The optional flags argument
//! specifies any ZeroMQ flags to use (e.g. ZMQ_DONTWAIT, ZMQ_SNDMORE). The optional
//! identity_str argument is used to identify the destination identity when using
//! DEALER-ROUTER channels.
//!
//! \param[in] msg_size - size of message to send
//! \param[in] message - pointer to location of message to send
//! \param[in] flags - ZeroMQ message send flags (default value 0)
//! \param[in] identity_str - identity of the destination endpoint to use for ROUTER sockets
//!
void IpcChannel::send(size_t msg_size, void *message, int flags, const std::string& identity_str)
{

  // Set and send the destination identity for ROUTER type channels
  if (socket_type_ == ZMQ_ROUTER) {
    router_send_identity(identity_str);
  }

  // Determine the message size and copy into ZeroMQ message
  zmq::message_t msg(msg_size);
  memcpy(msg.data(), message, msg_size);

  // Send the message on the underlying socket with the requested flags
  socket_.send(msg, flags);
}

//! Send an identity message part on ROUTER channels
//!
//! This private method is used by ROUTER channels to send the identity
//! of the destination endpoint as the first part of a multi-part ZeroMQ
//! message. This is necessary to ensure that the sent message is routed
//! to the correct destination (e.g. a remote DEALER channel).
//!
//! \param[in] identity_str - destination identity
//!
void IpcChannel::router_send_identity(const std::string& identity_str)
{
  size_t identity_size = identity_str.size();
  zmq::message_t identity_msg(identity_size);
  memcpy(identity_msg.data(), identity_str.data(), identity_size);
  socket_.send(identity_msg, ZMQ_SNDMORE);
}

//! Receive a message on the channel
//!
//! This method receives and returns a string-like message on the IpcChannel.
//! For ROUTER-type channels, the string pointer argument has the incoming
//! message identity copied into it. If the argument is NULL, the identity will
//! not be copied.
//!
//! \param[out] identity_str - pointer to string to receive identity on ROUTER sockets
//! \return string containing the message
//!
const std::string IpcChannel::recv(std::string* identity_str)
{
  // For ROUTER channels, receive the required identity message part first and copy
  // into the specified string location if not NULL.
  if (socket_type_ == ZMQ_ROUTER) {
    zmq::message_t identity_msg;
    socket_.recv(&identity_msg);
    if (identity_str != NULL) {
      *identity_str = std::string(static_cast<char*>(identity_msg.data()), identity_msg.size());
    }
  }

  // Receive the message payload
  zmq::message_t msg;
  socket_.recv(&msg);

  // Return the message as a string
  return std::string(static_cast<char*>(msg.data()), msg.size());
}

//! Receive a raw message on the channel
//!
//! This method receives a raw message on the channel, copying it into the
//! buffer specified in the argument and returning the size of the message received.
//! For ROUTER-type channels, the string pointer argument has the incoming
//! message identity copied into it. If this argument is NULL, the identity will
//! not be copied.
//!
//! \param[out] msg_buf - pointer to buffer where message should be copied
//! \param[out] identity_str - pointer to string to receive identity on ROUTER sockets
//! \return size of the received message
//!
const std::size_t IpcChannel::recv_raw(void *msg_buf, std::string* identity_str)
{

  // For ROUTER channels, receive the required identity message part first and copy
  // into the specified string location if not NULL.
  if (socket_type_ == ZMQ_ROUTER) {
    zmq::message_t identity_msg;
    socket_.recv(&identity_msg);
    if (identity_str != NULL) {
      *identity_str = std::string(static_cast<char*>(identity_msg.data()), identity_msg.size());
    }
  }

  // Receive the message payload
  std::size_t msg_size;
  zmq::message_t msg;
  socket_.recv(&msg);

  // Copy the message into the specified buffer and return the size
  msg_size = msg.size();
  memcpy(msg_buf, msg.data(), msg_size);

  return msg_size;
}

//! Set an option on the channel socket
//!
//! This method sets a ZeroMQ option on the socket underlying the IpcChannel.
//!
//! \param[in] option - ZeroMQ option to set
//! \param[in] option_value - pointer to option value to use
//! \param[in] option_len - length of option value to use in bytes
//!
void IpcChannel::setsockopt(int option, const void *option_value, std::size_t option_len)
{
  socket_.setsockopt(option, option_value, option_len);
}

//! Get an option from the channel socket
//!
//! This method gets a ZeroMQ option from the socket underlying the IpcChannel.
//!
//! \param[in] option - ZeroMQ option to get
//! \param[out] option_value - pointer to option value to use
//! \param[in,out] option_len - length of option value to use in bytes
//!
void IpcChannel::getsockopt(int option, void *option_value, std::size_t *option_len)
{
  socket_.getsockopt(option, option_value, option_len);
}

//! Determine if the end-of-message has been reached on a channel.
//!
//! This methods determines if the end-of-message has been reached on a channel, i.e.
//! there are no more parts of a multipart message to be received.
//!
//! \return boolean true if end-of-message reached
//!
bool IpcChannel::eom(void)
{
  bool eom = true;

  // GET the ZMQ_RCVMORE option value from the socket
  int more;
  std::size_t more_size = sizeof(more);
  socket_.getsockopt(ZMQ_RCVMORE, &more, &more_size);

  // If more data available, set EOM false
  if (more){
    eom = false;
  }
  return eom;
}

//! Poll the channel for incoming messages
//!
//! This method uses the ZeroMQ poll I/O multiplexing to to a timed poll on a channel.
//! This is primarily intended for testing, as normally IpcChannels are multiplexed into
//! an IpcReactor event loop.
//!
//! \param[in] timeout_ms - poll timeout in milliseconds
//! \return boolean true if there is incoming data ready on the socket
//!
bool IpcChannel::poll(long timeout_ms)
{
  zmq::pollitem_t pollitems[] = {{socket_, 0, ZMQ_POLLIN, 0}};

  zmq::poll(pollitems, 1, timeout_ms);

  return (pollitems[0].revents & ZMQ_POLLIN);
}

//! Close the IpcChannel socket
//!
//! This method closes the IpcChannel socket. Note that any subsequent attempt to use
//! the channel without re-initialisation will generate errors.
//!
void IpcChannel::close(void)
{
  socket_.close();
}
