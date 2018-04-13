/*!
 * FrameReceiverRxThread.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERRXTHREAD_H_
#define FRAMERECEIVERRXTHREAD_H_

#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"
#include "logging.h"

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"
#include "FrameDecoder.h"
#include "FrameReceiverConfig.h"
#include "OdinDataException.h"

using namespace OdinData;

namespace FrameReceiver
{

  const std::string RX_THREAD_ID = "RX_THREAD";

class FrameReceiverRxThreadException : public OdinData::OdinDataException
{
public:
  FrameReceiverRxThreadException(const std::string what) : OdinData::OdinDataException(what) { };
};

class FrameReceiverRxThread
{
public:
  FrameReceiverRxThread(FrameReceiverConfig& config, SharedBufferManagerPtr buffer_manager,
      FrameDecoderPtr frame_decoder, unsigned int tick_period_ms=100);
  virtual ~FrameReceiverRxThread();

  bool start();
  void stop();

  void frame_ready(int buffer_id, int frame_number);

protected:
  virtual void run_specific_service(void) = 0;
  virtual void cleanup_specific_service(void) = 0;

  void set_thread_init_error(const std::string& msg);

  void register_socket(int socket_fd, ReactorCallback callback);

  FrameReceiverConfig&   config_;
  IpcReactor             reactor_;

private:

  void run_service(void);

  void advertise_identity(void);
  void request_buffer_precharge(void);
  void handle_rx_channel(void);
  void tick_timer(void);
  void buffer_monitor_timer(void);
  void fill_status_params(IpcMessage& status_msg);

  LoggerPtr              logger_;              //!< Pointer to the logging facility
  SharedBufferManagerPtr buffer_manager_;      //!< Pointer to the shared buffer manager
  FrameDecoderPtr        frame_decoder_;       //!< Pointer to the frame decoder
  unsigned int           tick_period_ms_;      //!< Receiver thread tick timer period

  boost::shared_ptr<boost::thread> rx_thread_; //!< Pointer to RX thread
  
  IpcChannel             rx_channel_;          //!< Channel for communication with the main thread
  std::vector<int>       recv_sockets_;        //!< List of receive socket file descriptors

  bool                   run_thread_;          //!< Flag signalling thread should run
  bool                   thread_running_;      //!< Flag singalling if thread is running
  bool                   thread_init_error_;   //!< Flag singalling thread initialisation error
  std::string            thread_init_msg_;     //!< Thread initialisation message

};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERRXTHREAD_H_ */
