/*
 * IFrameCallback.cpp
 *
 *  Created on: 1 Jun 2016
 *      Author: gnx91527
 */

#include <IFrameCallback.h>

namespace FrameProcessor
{

  /** Construct a new IFrameCallback object.
   *
   * The constructor creates the new WorkQueue object.
   */
  IFrameCallback::IFrameCallback() :
    thread_(0),
    working_(false)
  {
    // Create the work queue for message offload
    queue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > >(new WorkQueue<boost::shared_ptr<Frame> >);
  }

  /**
   * Destructor
   */
  IFrameCallback::~IFrameCallback()
  {
  }

  /** Return the pointer to the WorkQueue.
   *
   * \return a pointer to the WorkQueue owned by this IFrameCallback class.
   */
  boost::shared_ptr<WorkQueue<boost::shared_ptr<Frame> > > IFrameCallback::getWorkQueue()
  {
    return queue_;
  }

  /** Start the worker thread.
   *
   * Check to ensure this object is not already working.  If it isn't then
   * create the new worker thread, binding the workerTask method to the thread execution.
   */
  void IFrameCallback::start()
  {
    if (!working_){
      // Set the working flag to true
      working_ = true;
      // Now start the worker thread to monitor the queue
      thread_ = new boost::thread(&IFrameCallback::workerTask, this);
    }
  }

  /** Stop the worker thread.
   *
   * Check this object is working.  Set the working_ flag to false and then
   * send an empty Frame pointer to the WorkerQueue object that will result
   * in the thread terminating gracefully.
   */
  void IFrameCallback::stop()
  {
    if (working_){
      // Set the working flag to false
      working_ = false;
      // Now notify the work queue we have finished by adding a null ptr
      boost::shared_ptr<Frame> nullMsg;
      queue_->add(nullMsg);
    }
  }

  /** Return whether our main thread is running.
   *
   */
  bool IFrameCallback::isWorking() const
  {
    return working_;
  }

  /** Record the name of an object that this IFrameCallback has registered with.
   *
   * Add the name to the map of registrations.
   *
   * \param[in] name - name of the object that this class has registered with.
   */
  void IFrameCallback::confirmRegistration(const std::string& name)
  {
    // Add the name of the frame source to our container
    registrations_[name] = "valid";
  }

  /** Remove the name from the confirmed registrations map.
   *
   * Remove the name from the map of registrations.
   *
   * \param[in] name - name of the object that this class has is no longer registered with.
   */
  void IFrameCallback::confirmRemoval(const std::string& name)
  {
    // Remove the name of the frame source from our container
    registrations_.erase(name);
  }

  /** Main thread of execution for this class.
   *
   * The thread executes in a continuous loop until the working_ flag is set to false.
   * The thread blocks on the remove call of the WorkQueue, waiting until a new Frame
   * is available.  As soon as a Frame becomes available the remove call returns and this
   * method calls the callback method (which is pure virtual and must be implemented by
   * a subclass).
   */
  void IFrameCallback::workerTask()
  {
    // Main worker task of this callback
    // Check the queue for messages
    while (working_){
      boost::shared_ptr<Frame> msg = queue_->remove();
      if (msg){
        // Once we have a message, call the callback
        this->callback(msg);
      }
    }
  }

} /* namespace FrameProcessor */
