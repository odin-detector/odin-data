/*
 * FrameReceiverController.h
 *
 *  Created on: Oct 10, 2017
 *      Author: tcn45
 */

#ifndef FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_
#define FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_

#include <log4cxx/logger.h>

#include "logging.h"
#include "DebugLevelLogger.h"

#include "zmq/zmq.hpp"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "FrameReceiverConfig.h" // TODO REMOVE THIS
#include "FrameReceiverRxThread.h"
#include "FrameReceiverUDPRxThread.h"
#include "FrameReceiverZMQRxThread.h"
#include "FrameDecoder.h"
#include "OdinDataException.h"
#include "ClassLoader.h"

using namespace OdinData;

namespace FrameReceiver
{

  class FrameReceiverController
  {

  public:

    FrameReceiverController ();
    virtual ~FrameReceiverController ();
    void configure(FrameReceiverConfig& config, OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);
    void run(void);
    void stop(void);

    void initialise_ipc_channels(void);
    void cleanup_ipc_channels(void);
    void initialise_frame_decoder(void);
    void initialise_buffer_manager(void);
    void precharge_buffers(void);
    void notify_buffer_config(const bool deferred=false);

    void handle_ctrl_channel(void);
    void handle_rx_channel(void);
    void handle_frame_release_channel(void);


  private:

    /** Pointer to the logging facility */
    log4cxx::LoggerPtr logger_;
    boost::scoped_ptr<FrameReceiverRxThread> rx_thread_;       //!< Receiver thread object
    FrameDecoderPtr                          frame_decoder_;   //!< Frame decoder object
    SharedBufferManagerPtr                   buffer_manager_;  //!< Buffer manager object

    FrameReceiverConfig config_;          //!< Configuration storage object TODO REMOVE THIS!

    static bool terminate_controller_;

    IpcChannel rx_channel_;
    IpcChannel ctrl_channel_;
    IpcChannel frame_ready_channel_;
    IpcChannel frame_release_channel_;

    IpcReactor reactor_;

    unsigned int frames_received_;
    unsigned int frames_released_;

  };

} /* namespace FrameReceiver */

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_ */
