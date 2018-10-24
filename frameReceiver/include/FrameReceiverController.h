/*
 * FrameReceiverController.h
 *
 *  Created on: Oct 10, 2017
 *      Author: tcn45
 */

#ifndef FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_
#define FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_

#include <set>
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
#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"
#include "FrameReceiverUDPRxThread.h"
#include "FrameReceiverZMQRxThread.h"
#include "FrameDecoder.h"
#include "OdinDataException.h"
#include "ClassLoader.h"

// Uncomment this to compile a diagnostic tick timer into the controller
//#define FR_CONTROLLER_TICK_TIMER

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

    FrameReceiverController (FrameReceiverConfig& config);
    virtual ~FrameReceiverController ();
    void configure(OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);
    void run(void);
    void stop(const bool deferred=false);

  private:

    void configure_ipc_channels(OdinData::IpcMessage& config_msg);
    void setup_control_channel(const std::string& ctrl_endpoint);
    void setup_rx_channel(const std::string& ctrl_endpoint);
    void setup_frame_ready_channel(const std::string& ctrl_endpoint);
    void setup_frame_release_channel(const std::string& ctrl_endpoint);
    void unbind_channel(OdinData::IpcChannel* channel, std::string& endpoint,
        const bool deferred=false);
    void cleanup_ipc_channels(void);

    void configure_frame_decoder(OdinData::IpcMessage& config_msg);
    void configure_buffer_manager(OdinData::IpcMessage& config_msg);
    void configure_rx_thread(OdinData::IpcMessage& config_msg);
    void stop_rx_thread(void);

    void handle_ctrl_channel(void);
    void handle_rx_channel(void);
    void handle_frame_release_channel(void);

    void precharge_buffers(void);
    void notify_buffer_config(const bool deferred=false);
    void store_rx_thread_status(OdinData::IpcMessage& rx_status_msg);
    void get_status(OdinData::IpcMessage& status_reply);
    void request_configuration(OdinData::IpcMessage& config_reply);

#ifdef FR_CONTROLLER_TICK_TIMER
    void tick_timer(void);
#endif

    log4cxx::LoggerPtr                       logger_;          //!< Pointer to the logging facility
    boost::scoped_ptr<FrameReceiverRxThread> rx_thread_;       //!< Receiver thread object
    FrameDecoderPtr                          frame_decoder_;   //!< Frame decoder object
    SharedBufferManagerPtr                   buffer_manager_;  //!< Buffer manager object

    FrameReceiverConfig& config_;         //!< Configuration storage object
    bool terminate_controller_;           //!< Flag to signal termination of the controller

    bool need_ipc_reconfig_;              //!< Flag to signal reconfiguration of IPC channels
    bool need_decoder_reconfig_;          //!< Flag to signal reconfiguration of frame decoder
    bool need_rx_thread_reconfig_;        //!< Flag to signal reconfiguration of RX thread
    bool need_buffer_manager_reconfig_;   //!< Flag to signal reconfiguration of buffer manager

    bool ipc_configured_;                 //!< Indicates that IPC channels are configured
    bool decoder_configured_;             //!< Indicates that the frame decoder is configured
    bool buffer_manager_configured_;      //!< Indicates that the buffer manager is configured
    bool rx_thread_configured_;           //!< Indicates that the RX thread is configured
    bool configuration_complete_;         //!< Indicates that all components are configured
    
    IpcContext& ipc_context_;             //!< ZMQ context for IPC channels
    IpcChannel rx_channel_;               //!< Channel for communication with receiver thread
    IpcChannel ctrl_channel_;             //!< Channel for communication with  control clients
    IpcChannel frame_ready_channel_;      //!< Channel for signalling to downstream processes
    IpcChannel frame_release_channel_;    //!< Channel for receiving notification of released frames

    IpcReactor reactor_;                  //!< Reactor for multiplexing all communications

    unsigned int frames_received_;        //!< Counter for frames received
    unsigned int frames_released_;        //!< Counter for frames released

    std::string rx_thread_identity_;      //!< Identity of the RX thread dealer channel

    boost::scoped_ptr<OdinData::IpcMessage> rx_thread_status_; //!< Status of the receiver thread

  };

  const std::size_t deferred_action_delay_ms = 1000; //!< Default delay in ms for deferred actions

} /* namespace FrameReceiver */

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_ */
