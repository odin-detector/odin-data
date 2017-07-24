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
#include "SharedBufferManager.h"

namespace FrameProcessor
{

/**
 * The SharedMemoryController class uses an IpcReactor object which is used
 * to notify this class when new data is available from the
 * frame receiver service. This class also owns an instance of the
 * SharedMemoryParser class, which extracts the data from the shared memory
 * location specified by the incoming IpcMessage objects, constructs a
 * Frame to contain the data and meta data, and then notifies any listening
 * plugins. This class also notifies the frame receiver service once the
 * shared memory location is available for re-use.
 */
class SharedMemoryController
{
public:
  SharedMemoryController(boost::shared_ptr<OdinData::IpcReactor> reactor, const std::string& rxEndPoint, const std::string& txEndPoint);
  virtual ~SharedMemoryController();
  void setSharedBufferManager(const std::string& shared_buffer_name);
  void registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb);
  void removeCallback(const std::string& name);
  void handleRxChannel();

private:
  /** Pointer to logger */
  LoggerPtr logger_;
  /** Pointer to SharedBufferManager object */
  boost::shared_ptr<OdinData::SharedBufferManager> sbm_;
  /** Map of IFrameCallback pointers, indexed by name */
  std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
  /** IpcReactor pointer, for managing IpcMessage objects */
  boost::shared_ptr<OdinData::IpcReactor> reactor_;
  /** IpcChannel for receiving notifications of new frames */
  OdinData::IpcChannel             rxChannel_;
  /** IpcChannel for sending notifications of frame release */
  OdinData::IpcChannel             txChannel_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_ */
