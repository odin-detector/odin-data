/*
 * IJSONCallback.h
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_IJSONCALLBACK_H_
#define TOOLS_FILEWRITER_IJSONCALLBACK_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "JSONMessage.h"
#include "WorkQueue.h"

namespace filewriter
{

  class IJSONCallback
  {
  public:
    IJSONCallback();
    virtual ~IJSONCallback();
    boost::shared_ptr<WorkQueue<boost::shared_ptr<JSONMessage> > > getWorkQueue();
    void start();

  private:
    LoggerPtr logger_;
    boost::thread *thread_;
    boost::shared_ptr<WorkQueue<boost::shared_ptr<JSONMessage> > > queue_;
    bool working_;

    void workerTask();
    virtual void callback(boost::shared_ptr<JSONMessage> msg) = 0;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_IJSONCALLBACK_H_ */
