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

namespace FrameProcessor
{
  /** Interface to provide producer/consumer base processing of Frame objects.
   *
   * The IFrameCallback class is a pure virtual class (interface) that must be
   * subclassed and the callback method overridden for use.  It provides a WorkQueue
   * for Frame object pointers that allow plugin chains to be created which can
   * each process the Frame object within their own thread.
   */
  class IFrameCallback
  {
  public:
    IFrameCallback();
    virtual ~IFrameCallback();
    boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > getWorkQueue();
    void start();
    void stop();
    bool isWorking() const;
    void confirmRegistration(const std::string& name);
    void confirmRemoval(const std::string& name);
  
    /** Callback for when ever a new Frame is available.
     *
     * Pure virtual method, must be implemented by subclass.  This will be called
     * whenever a new Frame is placed onto the WorkQueue.
     *
     * \param[in] frame - pointer to Frame object ready for processing by the IFrameCallback subclass.
     */
    virtual void callback(boost::shared_ptr<Frame> frame) = 0;

  private:
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Pointer to worker queue thread */
    boost::thread *thread_;
    /** Pointer to WorkQueue for Frame object pointers */
    boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > queue_;
    /** Is this IFrameCallback working */
    bool working_;
    /** Map of confirmed registrations to this worker queue */
    std::map<std::string, std::string> registrations_;

    void workerTask();
  };

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_IFRAMECALLBACK_H_ */
