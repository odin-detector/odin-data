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

namespace FrameReceiver
{
	class FrameReceiverApp
	{
	public:
		FrameReceiverApp();
		~FrameReceiverApp();

		int parseArguments(int argc, char** argv);

		void run(void);
		static void stop(void);

	private:

		LoggerPtr    mLogger;

	};
}


#endif /* FRAMERECEIVERAPP_H_ */
