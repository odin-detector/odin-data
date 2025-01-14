/*!
 * FrameReceiverRxThread.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERZMQRXTHREAD_H_
#define FRAMERECEIVERZMQRXTHREAD_H_

#include <boost/asio.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoderZMQ.h"
#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"
#include "OdinDataException.h"

using namespace OdinData;

namespace FrameReceiver
{
class FrameReceiverZMQRxThread : public FrameReceiverRxThread
{
public:
  FrameReceiverZMQRxThread(FrameReceiverConfig& config, SharedBufferManagerPtr buffer_manager,
      FrameDecoderPtr frame_decoder, unsigned int tick_period_ms=100);
  virtual ~FrameReceiverZMQRxThread();

private:
  void run_specific_service(void);
  void cleanup_specific_service(void);

  void handle_receive_socket();

  LoggerPtr          logger_;
  IpcChannel         skt_channel_;
  FrameDecoderZMQPtr frame_decoder_;
};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERRXTHREAD_H_ */
