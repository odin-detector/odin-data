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

namespace FrameReceiver
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
        void connect(const char* endpoint);

        void send(std::string& message_str);
        void send(const char* message);

        std::string recv(void);
        bool poll(long timeout_ms = -1);

    private:

        IpcContext& context_;
        zmq::socket_t socket_;


    };
} // namespace FrameReceiver



#endif /* IPCCHANNEL_H_ */
