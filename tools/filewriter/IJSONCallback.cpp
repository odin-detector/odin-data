/*
 * IJSONCallback.cpp
 *
 *  Created on: 27 May 2016
 *      Author: gnx91527
 */

#include <IJSONCallback.h>

namespace filewriter
{

  IJSONCallback::IJSONCallback() :
      thread_(0),
      working_(false)
  {
    // Create the work queue for message offload
    queue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<JSONMessage> > >(new WorkQueue<boost::shared_ptr<JSONMessage> >);
  }

  IJSONCallback::~IJSONCallback ()
  {
  }

  boost::shared_ptr<WorkQueue<boost::shared_ptr<JSONMessage> > > IJSONCallback::getWorkQueue()
  {
    return queue_;
  }

  void IJSONCallback::start()
  {
    // Set the working flag to true
    working_ = true;
    // Now start the worker thread to monitor the queue
    thread_ = new boost::thread(&IJSONCallback::workerTask, this);
  }

  void IJSONCallback::workerTask()
  {
    // Main worker task of this callback
    // Check the queue for messages
    while (working_){
      boost::shared_ptr<JSONMessage> msg = queue_->remove();
      // Once we have a message, call the callback
      this->callback(msg);
    }
  }

} /* namespace filewriter */
