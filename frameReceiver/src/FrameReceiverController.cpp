/*
 * FrameReceiverController.cpp - controller class for the FrameReceiver application
 *
 *  Created on: Oct 10, 2017
 *      Author: tcn45
 */

#include "FrameReceiverController.h"

namespace FrameReceiver
{

  FrameReceiverController::FrameReceiverController () :
      logger_(log4cxx::Logger::getLogger("FR.FrameReceiverController"))
  {
    // TODO Auto-generated constructor stub

  }

  FrameReceiverController::~FrameReceiverController ()
  {
    // TODO Auto-generated destructor stub
  }

  void FrameReceiverController::configure(OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply)
  {
    LOG4CXX_DEBUG(logger_, "Configuration submitted: " << config_msg.encode());
  }

} /* namespace FrameReceiver */
