/*!
 * IpcChannel.h
 *
 *  Created on: Feb 6, 2015
 *      Author: tcn45
 */

#ifndef IPCCHANNEL_H_
#define IPCCHANNEL_H_

#include "zmq/zmq.hpp"
#include <iostream>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace OdinData
{

    class IpcContext
    {
    public:
        static IpcContext& Instance(void);
        zmq::context_t& get(void);

    private:
        IpcContext(int io_threads=1);
        IpcContext(const IpcContext&);
        IpcContext& operator=(const IpcContext&);

        zmq::context_t zmq_context_;
    };

    class IpcChannel
    {
    public:

        IpcChannel(int type);
        ~IpcChannel();
        void bind(const char* endpoint);
        void bind(std::string& endpoint);
        void connect(const char* endpoint);
        void connect(std::string& endpoint);

        void subscribe(const char* topic);

        void send(std::string& message_str);
        void send(const char* message);

        const std::string recv(void);

        bool poll(long timeout_ms = -1);
        void close(void);

        friend class IpcReactor;

    private:

        IpcContext& context_;
        zmq::socket_t socket_;


    };

} // namespace OdinData



#endif /* IPCCHANNEL_H_ */
