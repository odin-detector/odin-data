/*!
 * FrameReceiverRxThread.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERUDPRXTHREAD_H_
#define FRAMERECEIVERUDPRXTHREAD_H_

#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoderUDP.h"

#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"
#include "OdinDataException.h"

using namespace OdinData;

namespace FrameReceiver
{
    class FrameReceiverUDPRxThread : public FrameReceiverRxThread
    {
    public:
        FrameReceiverUDPRxThread(FrameReceiverConfig& config, LoggerPtr& logger,
                SharedBufferManagerPtr buffer_manager, FrameDecoderPtr frame_decoder,
                unsigned int tick_period_ms=100);
        virtual ~FrameReceiverUDPRxThread();

    private:

        void run_specific_service(void);
        void cleanup_specific_service(void);

        void handle_receive_socket(int socket_fd, int recv_port);

        LoggerPtr              logger_;
        FrameDecoderUDPPtr     frame_decoder_;

    };

} // namespace FrameReceiver

#endif /* FRAMERECEIVERRXTHREAD_H_ */
