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

#include <boost/shared_ptr.hpp>

#include "DebugLevelLogger.h"
#include "FrameReceiverConfig.h"
#include "FrameReceiverController.h"

using namespace OdinData;

namespace FrameReceiver
{

//! Frame receiver application class/
//!
//! This class implements the main functionality of the FrameReceiver application, parsing command line
//! and configuraiton file options before creating, configuring and running the controller.

class FrameReceiverApp
{

public:

  FrameReceiverApp();
  ~FrameReceiverApp();

  int parse_arguments(int argc, char** argv);

  void run(void);
  static void stop(void);

private:

  LoggerPtr logger_;                    //!< Log4CXX logger instance pointer
  FrameReceiverConfig config_;          //!< Configuration storage object
  std::string json_config_file_;        //!< Full path to JSON configuration file
  static boost::shared_ptr<FrameReceiverController> controller_; //!< FrameReceiver controller object

};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERAPP_H_ */
