/*!
 * IpcChannel.h
 *
 *  Created on: Feb 6, 2015
 *      Author: tcn45
 */

#ifndef IPCCHANNEL_H_
#define IPCCHANNEL_H_

#include <iostream>
#include <map>

#include "zmq/zmq.hpp"
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>

namespace OdinData
{

class IpcContext
{
public:
  static IpcContext& Instance(unsigned int io_threads=1);
  zmq::context_t& get(void);

private:
  IpcContext(unsigned int io_threads=1);
  IpcContext(const IpcContext&);
  IpcContext& operator=(const IpcContext&);

  zmq::context_t zmq_context_;
};

class IpcChannel
{
public:

  IpcChannel(int type);
  IpcChannel(int type, const std::string identity);
  ~IpcChannel();
  void bind(const char* endpoint);
  void bind(std::string& endpoint);
  void unbind(const char* endpoint);
  void unbind(const std::string& endpoint);
  void connect(const char* endpoint);
  void connect(std::string& endpoint);
  bool has_bound_endpoint(const std::string& endpoint);

  void subscribe(const char* topic);

  void send(std::string& message_str, int flags = 0,
    const std::string& identity_str = std::string());
  void send(const char* message, int flags = 0,
    const std::string& identity_str = std::string());
  void send(size_t msg_size, void *message, int flags = 0,
    const std::string& identity_str = std::string());

  const std::string recv(std::string* identity_str=0);
  const std::size_t recv_raw(void *msg_buf, std::string* identity_str=0);

  void setsockopt(int option, const void *option_value, std::size_t option_len);
  void getsockopt(int option, void *option_value, std::size_t *option_len);

  bool eom(void);
  bool poll(long timeout_ms = -1);
  void close(void);

  friend class IpcReactor;

private:

  void router_send_identity(const std::string& identity_str);

  IpcContext& context_;
  zmq::socket_t socket_;
  int socket_type_;
  std::map<std::string, std::string> bound_endpoints_;


};

} // namespace OdinData
#endif /* IPCCHANNEL_H_ */
