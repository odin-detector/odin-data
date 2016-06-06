/*
 * IFrameCallback.h
 *
 *  Created on: 1 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_IFRAMECALLBACK_H_
#define TOOLS_FILEWRITER_IFRAMECALLBACK_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "Frame.h"
#include "WorkQueue.h"

namespace filewriter
{

  class IFrameCallback
  {
  public:
    IFrameCallback();
    virtual ~IFrameCallback ();
    boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > getWorkQueue();
    void start();

  private:
    LoggerPtr logger_;
    boost::thread *thread_;
    boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > queue_;
    bool working_;

    void workerTask();
    virtual void callback(boost::shared_ptr<Frame> frame) = 0;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_IFRAMECALLBACK_H_ */
