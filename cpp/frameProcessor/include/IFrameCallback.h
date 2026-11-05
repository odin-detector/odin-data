/*
 * IFrameCallback.h
 *
 *  Created on: 1 Jun 2016
 *      Author: gnx91527
 */

#ifndef IFRAMECALLBACK_H_
#define IFRAMECALLBACK_H_

#include <atomic>
#include <boost/shared_ptr.hpp>
#include <thread>

#include "Frame.h"
#include "WorkQueue.h"

namespace FrameProcessor {
/** Interface to provide producer/consumer base processing of Frame objects.
 *
 * The IFrameCallback class is a pure virtual class (interface) that must be
 * subclassed and the callback method overridden for use. It provides a WorkQueue
 * for Frame object pointers that allow plugin chains to be created which can
 * each process the Frame object within their own thread.
 */
class IFrameCallback {
public:
    /** Construct a new IFrameCallback object */
    IFrameCallback() = default;
    ~IFrameCallback() = default;
    WorkQueue<boost::shared_ptr<Frame>>& getWorkQueue()
    {
        return queue_;
    }
    /** Start the worker thread.
     *
     * Check to ensure this object is not already working. If it isn't then
     * create the new worker thread, binding the workerTask method to the thread execution.
     */
    void start()
    {
        // Set the run condition to true
        run_.store(true, std::memory_order::memory_order_release);
        // Now start the worker thread to monitor the queue
        thread_ = std::thread(&IFrameCallback::workerTask, this);
    }
    /** Stop the worker thread.
     * Check this object is working. Set the run_ flag to false and then
     * send an empty Frame pointer to the WorkerQueue object that will result
     * in the thread terminating gracefully. The thread calling stop()
     * will then BLOCK on thread::join() until the Worker thread exists gracefully.
     */
    void stop()
    {
        // Set the run condition flag to false
        run_.store(false);
        // Now notify the work queue we have finished by adding a nullptr
        boost::shared_ptr<Frame> nullMsg;
        queue_.add(nullMsg);
        thread_.join();
    }

    /** Callback for when ever a new Frame is available.
     *
     * Pure virtual method, must be implemented by subclass. This will be called
     * whenever a new Frame is placed onto the WorkQueue.
     *
     * \param[in] frame - pointer to Frame object ready for processing by the IFrameCallback subclass.
     */
    virtual void callback(boost::shared_ptr<Frame> frame) = 0;

private:
    /** Pointer to WorkQueue for Frame object pointers */
    WorkQueue<boost::shared_ptr<Frame>> queue_;
    /** IFrameCallback run flag */
    std::atomic<bool> run_;
    /** Pointer to worker queue thread */
    std::thread thread_;
    /** Main thread of execution for this class.
     *
     * The thread executes in a continuous loop until the run_ flag is set to false.
     * The thread blocks on the remove call of the WorkQueue, waiting until a new Frame
     * is available. As soon as a Frame becomes available the remove call returns and this
     * method calls the callback method (which is pure virtual and must be implemented by
     * a subclass).
     */
    void workerTask()
    {
        // Check the queue for messages
        while (run_.load(std::memory_order::memory_order_relaxed)) {
            boost::shared_ptr<Frame> msg = queue_.remove();
            if (msg) {
                // Once we have a message, call the callback
                this->callback(msg);
            }
        }
    }
};

} /* namespace FrameProcessor */

#endif /* IFRAMECALLBACK_H_ */
