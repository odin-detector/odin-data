/*
 * IFrameCallback.cpp
 *
 *  Created on: 1 Jun 2016
 *      Author: gnx91527
 */

#include <IFrameCallback.h>

namespace filewriter
{

  IFrameCallback::IFrameCallback() :
    thread_(0),
    working_(false)
  {
    // Create the work queue for message offload
    queue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > >(new WorkQueue<boost::shared_ptr<Frame> >);
  }

  IFrameCallback::~IFrameCallback()
  {
  }

  boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > IFrameCallback::getWorkQueue()
  {
    return queue_;
  }

  void IFrameCallback::start()
  {
    // Set the working flag to true
    working_ = true;
    // Now start the worker thread to monitor the queue
    thread_ = new boost::thread(&IFrameCallback::workerTask, this);
  }

  void IFrameCallback::workerTask()
  {
    // Main worker task of this callback
    // Check the queue for messages
    while (working_){
      boost::shared_ptr<Frame> msg = queue_->remove();
      // Once we have a message, call the callback
      this->callback(msg);
    }
  }

} /* namespace filewriter */
