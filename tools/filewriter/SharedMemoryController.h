/*
 * SharedMemoryController.h
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_
#define TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "boost/date_time/posix_time/posix_time.hpp"

#include "IFrameCallback.h"
#include "IpcReactor.h"
#include "IpcChannel.h"
#include "IpcMessage.h"
#include "SharedMemoryParser.h"

namespace filewriter
{

  /**
   * The SharedMemoryController class implements an IJSONCallback interface
   * which is used to notify this class when new data is available from the
   * frame receiver service.  This class also owns an instance of the
   * SharedMemoryParser class, which extracts the data from the shared memory
   * location specified by the incoming JSONMessage objects, constructs a
   * frame to contain the data and meta data, and then notifies any listening
   * plugins.  This class also notifies the frame receiver service once the
   * shared memory location is available.
   */
  class SharedMemoryController
  {
  public:
    SharedMemoryController(boost::shared_ptr<FrameReceiver::IpcReactor> reactor, const std::string& rxEndPoint, const std::string& txEndPoint);
    virtual ~SharedMemoryController();
    void setSharedMemoryParser(boost::shared_ptr<SharedMemoryParser> smp);
    void registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb);
    void removeCallback(const std::string& name);
    void handleRxChannel();

  private:
    LoggerPtr logger_;
    boost::shared_ptr<SharedMemoryParser> smp_;
    //boost::shared_ptr<JSONPublisher> frp_;
    std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
    boost::shared_ptr<FrameReceiver::IpcReactor> reactor_;
    FrameReceiver::IpcChannel             rxChannel_;
    FrameReceiver::IpcChannel             txChannel_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_ */
