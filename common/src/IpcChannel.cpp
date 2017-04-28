/*!
q * IpcChannel.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: Tim Nicholls, STFC Application Engineering
 */

#include "IpcChannel.h"

using namespace OdinData;

IpcContext& IpcContext::Instance(void)
{
    static IpcContext ipcContext;
    return ipcContext;
}

zmq::context_t& IpcContext::get(void)
{
    return zmq_context_;
}

IpcContext::IpcContext(int io_threads) :
    zmq_context_(io_threads)
{
    // std::cout << "IpcContext constructor" << std::endl;
}

IpcChannel::IpcChannel(int type) :
    context_(IpcContext::Instance()),
    socket_(context_.get(), type)
{
    //std::cout << "IpcChannel constructor" << std::endl;
}

IpcChannel::~IpcChannel()
{
    //td::cout << "IpcChannel destructor" << std::endl;
}

void IpcChannel::bind(const char* endpoint)
{
    socket_.bind(endpoint);
}

void IpcChannel::bind(std::string& endpoint)
{
    this->bind(endpoint.c_str());
}

void IpcChannel::connect(const char* endpoint)
{
    socket_.connect(endpoint);
}

void IpcChannel::connect(std::string& endpoint)
{
    this->connect(endpoint.c_str());
}

void IpcChannel::subscribe(const char* topic)
{
    socket_.setsockopt(ZMQ_SUBSCRIBE, topic, strlen(topic));
}

void IpcChannel::send(std::string& message_str)
{
    size_t msg_size = message_str.size();
    zmq::message_t msg(msg_size);
    memcpy(msg.data(), message_str.data(), msg_size);
    socket_.send(msg);
}

void IpcChannel::send(const char* message)
{
    size_t msg_size = strlen(message);
    zmq::message_t msg(msg_size);
    memcpy(msg.data(), message, msg_size);
    socket_.send(msg);

}

const std::string IpcChannel::recv()
{
    zmq::message_t msg;
    socket_.recv(&msg);

    return std::string(static_cast<char*>(msg.data()), msg.size());
}

const std::size_t IpcChannel::recv_raw(void *dPtr)
{
    std::size_t msg_size;
    zmq::message_t msg;

    socket_.recv(&msg);
    msg_size = msg.size();
    memcpy(dPtr, msg.data(), msg_size);
    return msg_size;
}

bool IpcChannel::eom(void)
{
	bool eom = true;
	int64_t more;
	size_t more_size = sizeof(more);
	socket_.getsockopt(ZMQ_RCVMORE, &more, &more_size);
	if (more){
		eom = false;
	}
	return eom;
}

bool IpcChannel::poll(long timeout_ms)
{
    zmq::pollitem_t pollitems[] = {{socket_, 0, ZMQ_POLLIN, 0}};

    zmq::poll(pollitems, 1, timeout_ms);

    return (pollitems[0].revents & ZMQ_POLLIN);

}

void IpcChannel::close(void)
{
    socket_.close();
}

