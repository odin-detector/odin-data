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
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "zmq/zmq.hpp"

#include <boost/scoped_ptr.hpp>

#include "IpcChannel.h"
#include "FrameReceiverConfig.h"
#include "FrameReceiverRxThread.h"

namespace FrameReceiver
{
	//! Frame receiver application class/
	//!
	//! This class implements the main functionality of the FrameReceiver application, providing
	//! the overall framework for running the frame receiver, capturing frames of incoming data and
    //! handing them off to a processing appplication via shared memory. The application communicates
	//! with the downstream processing (and internally) via ZeroMQ inter-process channels.

	class FrameReceiverApp
	{
	public:
		FrameReceiverApp();
		~FrameReceiverApp();

		int parseArguments(int argc, char** argv);

		void run(void);
		static void stop(void);

	private:

		LoggerPtr             logger_;    //!< Log4CXX logger instance pointer
		FrameReceiverConfig   config_;    //!< Configuration storage object
		boost::scoped_ptr<FrameReceiverRxThread> rx_thread_; //!< Receiver thread object

		static bool run_frame_receiver_;

		//zmq::context_t zmq_context_;
		//zmq::socket_t  rx_thread_chan_;

		IpcChannel rx_channel_;
		IpcChannel ctrl_channel_;

	};
}


#endif /* FRAMERECEIVERAPP_H_ */
