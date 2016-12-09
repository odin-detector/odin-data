/*
 * FrameReceiver.h
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERAPP_H_
#define FRAMERECEIVERAPP_H_

#include <string>
using namespace std;

#include <time.h>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include "DebugLevelLogger.h"

#include "zmq/zmq.hpp"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"
#include "FrameDecoder.h"
#include "PercivalEmulatorFrameDecoder.h"
#include "FrameReceiverException.h"

namespace FrameReceiver
{

	//! Frame receiver application class/
	//!
	//! This class implements the main functionality of the FrameReceiver application, providing
	//! the overall framework for running the frame receiver, capturing frames of incoming data and
    //! handing them off to a processing application via shared memory. The application communicates
	//! with the downstream processing (and internally) via ZeroMQ inter-process channels.

	class FrameReceiverApp
	{
	public:
		FrameReceiverApp();
		~FrameReceiverApp();

		int parse_arguments(int argc, char** argv);

		void run(void);
		static void stop(void);


	private:

		void initialise_ipc_channels(void);
		void cleanup_ipc_channels(void);
        void initialise_frame_decoder(void);
        void initialise_buffer_manager(void);
        void precharge_buffers(void);

        void handle_ctrl_channel(void);
        void handle_rx_channel(void);
        void handle_frame_release_channel(void);
        void rx_ping_timer_handler(void);
        void timer_handler2(void);

		LoggerPtr             logger_;                           //!< Log4CXX logger instance pointer
		FrameReceiverConfig   config_;                           //!< Configuration storage object
		boost::scoped_ptr<FrameReceiverRxThread> rx_thread_;     //!< Receiver thread object
		FrameDecoderPtr frame_decoder_;          //!< Frame decoder object
		SharedBufferManagerPtr buffer_manager_;  //!< Buffer manager object

		static bool terminate_frame_receiver_;

		IpcChannel rx_channel_;
		IpcChannel ctrl_channel_;
		IpcChannel frame_ready_channel_;
		IpcChannel frame_release_channel_;

		IpcReactor reactor_;

		unsigned int frames_received_;
		unsigned int frames_released_;

	};
}


#endif /* FRAMERECEIVERAPP_H_ */
