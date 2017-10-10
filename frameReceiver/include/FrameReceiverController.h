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
#include "IpcMessage.h"
#include "IpcChannel.h"
#include "IpcReactor.h"

namespace FrameReceiver
{

  class FrameReceiverController
  {
  public:
    FrameReceiverController ();
    virtual ~FrameReceiverController ();
    void configure(OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);

  private:

    /** Pointer to the logging facility */
    log4cxx::LoggerPtr logger_;
  };

} /* namespace FrameReceiver */

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVERCONTROLLER_H_ */
