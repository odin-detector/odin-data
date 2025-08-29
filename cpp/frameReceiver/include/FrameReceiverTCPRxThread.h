/*!
 * FrameReceiverTCPRxThread.h
 *
 */

#ifndef FRAMERECEIVERTCPRXTHREAD_H_
#define FRAMERECEIVERTCPRXTHREAD_H_

#include <boost/asio.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoderTCP.h"
#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"
#include "OdinDataException.h"

using namespace OdinData;

namespace FrameReceiver
{
class FrameReceiverTCPRxThread : public FrameReceiverRxThread
{
public:
  FrameReceiverTCPRxThread(FrameReceiverConfig& config, SharedBufferManagerPtr buffer_manager,
      FrameDecoderPtr frame_decoder, unsigned int tick_period_ms=100);
  virtual ~FrameReceiverTCPRxThread();

private:
  void run_specific_service(void);
  void cleanup_specific_service(void);

  void handle_receive_socket(int socket_fd, int recv_port);

  LoggerPtr          logger_;
  FrameDecoderTCPPtr frame_decoder_;
  int recv_socket_;
};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERTCPRXTHREAD_H_ */
