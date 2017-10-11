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
#include "FrameReceiverException.h"
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

  //! Frame receiver controller class/
  //!
  //! This class implements the main controller of the FrameReceiver application, providing
  //! the overall framework for running the frame receiver, capturing frames of incoming data and
  //! handing them off to a processing application via shared memory. The controller communicates
  //! with the downstream processing (and internally) via ZeroMQ inter-process channels.

  class FrameReceiverController
  {

  public:

    FrameReceiverController ();
    virtual ~FrameReceiverController ();
    void configure(FrameReceiverConfig& config,
        OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);
    void run(void);
    void stop(void);

    void configure_ipc_channels(OdinData::IpcMessage& config_msg);
    void setup_control_channel(const std::string& ctrl_endpoint);
    void setup_rx_channel(const std::string& ctrl_endpoint);
    void setup_frame_ready_channel(const std::string& ctrl_endpoint);
    void setup_frame_release_channel(const std::string& ctrl_endpoint);
    void cleanup_ipc_channels(void);
    void configure_frame_decoder(OdinData::IpcMessage& config_msg);
    void configure_buffer_manager(OdinData::IpcMessage& config_msg);
    void configure_rx_thread(OdinData::IpcMessage& config_msg);
    void precharge_buffers(void);
    void notify_buffer_config(const bool deferred=false);

    void handle_ctrl_channel(void);
    void handle_rx_channel(void);
    void handle_frame_release_channel(void);


  private:

    log4cxx::LoggerPtr                       logger_;          //!< Pointer to the logging facility
    boost::scoped_ptr<FrameReceiverRxThread> rx_thread_;       //!< Receiver thread object
    FrameDecoderPtr                          frame_decoder_;   //!< Frame decoder object
    SharedBufferManagerPtr                   buffer_manager_;  //!< Buffer manager object

    FrameReceiverConfig config_;          //!< Configuration storage object TODO REMOVE THIS!
    bool terminate_controller_;           //!< Flag to signal temination of the controller

    IpcChannel rx_channel_;               //!< Channel for communication with receiver thread
    IpcChannel ctrl_channel_;             //!< Channel for communication with external control clients
    IpcChannel frame_ready_channel_;      //!< Channel for signalling frames ready to downstream processes
    IpcChannel frame_release_channel_;    //!< Channel for receiving notification of released frames

    IpcReactor reactor_;                  //!< Reactor for multiplexing all communications

    unsigned int frames_received_;        //!< Counter for frames received
    unsigned int frames_released_;        //!< Counter for frames released

  };

} /* namespace FrameReceiver */

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_ */
