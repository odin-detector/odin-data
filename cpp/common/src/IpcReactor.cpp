/*!
 * IpcReactor.cpp - implementation of the IpcReactor and associated classes
 *
 *  Created on: Feb 16, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "IpcReactor.h"
#include "gettime.h"

using namespace OdinData;

//! Constructor - instantiates an IpcReactorTimer object
//!
//! This instantiates and IpcReactorTimer object for use in an IpcReactor. The timer is
//! not responsible for firing itself; rather it is a tracking object with the real work
//! being performed by the IpcReactor event loop. The timer runs periodically, either
//! forever or for a fixed number of times.
//!
//! \param delay_ms Timer delay in milliseconds for
//! \param times    Number of times the timer should fire, a value of 0 indicates forever
//! \param callback Callback function signature to be called when the timer fires

IpcReactorTimer::IpcReactorTimer(size_t delay_ms, size_t times, TimerCallback callback) :
    timer_id_(last_timer_id_++),
    delay_ms_(delay_ms),
    times_(times),
    callback_(callback),
    when_(clock_mono_ms() + delay_ms),
    expired_(false)
{
}

//! Destructor - destroys an IpcReactorTimer object
IpcReactorTimer::~IpcReactorTimer()
{
}

//! Returns the unique ID of the timer instance
//!
//! This method returns the unique ID of the timer, which can be used to track and remove
//! the timer from the reactor as necessary
//!
//! \return int unique timer id

int IpcReactorTimer::get_id(void)
{
  return timer_id_;
}

//! Executes the registered callback method of the timer
//!
//! This method executes the registered callback method of the timer and then
//! evaluates if the timer should fire again. If so, the time of the next firing
//! is updated according to the specified delay

void IpcReactorTimer::do_callback(void)
{
  // Call the registered callback function
  this->callback_();

  // If the timer has a finite numnber of times to run, decrement and set
  // the expired flag if necessary, otherwise reset the when variable to
  // indicate the next time it should fire
  if (times_ > 0 && --times_ == 0)
  {
    expired_ = true;
  }
  else
  {
    when_ += delay_ms_;
  }
}

//! Indicates if the timer has fired (e.g. is due for handling)
//!
//! This method indicates if the timer has fired, i.e. is due for handling by the
//! controlling object.
//!
//! \return boolean value, true if timer has fired

bool IpcReactorTimer::has_fired(void)
{
  return (clock_mono_ms() >= when_);
}

//! Indicates if the timer has expired, i.e. reached its maximum times fired
//!
//! This method indicates if the timer has expired, i.e. reached its maximum
//! number of times fired and is no longer active. It is up to the controlling
//! object to delete the timer as appropriate.
//!
//! \return boolean value, true if the timer has fired
bool IpcReactorTimer::has_expired(void)
{
  return expired_;
}

//! Indicates when (in absolute monotonic time) the timer is due to fire
//!
//! this method indicates when a timer is next due to fire, in absolute
//! monotonic time in milliseconds
//!
//! \return TimeMs value indicating when the timer is next due to fire

TimeMs IpcReactorTimer::when(void)
{
  return when_;
}

//! Returns the current monotonic clock time in milliseconds (static method)
//!
//! This static method returns the current monotonic system clock time to
//! millisecond precision. The monotonic clock is used rather than the local
//! real time, since the latter can be reset by clock operations on the host
//! system, which would disrupt the periodic operation of any timers
//!
//! \return TimeMs value of the current monotonic time in milliseconds
TimeMs IpcReactorTimer::clock_mono_ms(void)
{
  struct timespec ts;
  gettime(&ts, true);

  return (TimeMs)((TimeMs) ts.tv_sec * 1000 + (TimeMs) ts.tv_nsec / 1000000);
}

// Initialise static class variable holding last unique timer ID assigned
int IpcReactorTimer::last_timer_id_ = 0;

//! Constructor
//!
//! This constructs an IpcReactor object ready for use

IpcReactor::IpcReactor() :
    terminate_reactor_(false),
    pollitems_(0),
    callbacks_(0),
    pollsize_(0),
    needs_rebuild_(true)
{
}

//! Destructor
//!
//! This destroys an IpcReactor object

IpcReactor::~IpcReactor()
{
  delete[] pollitems_;
  delete[] callbacks_;
}

//! Adds an IPC channel and associated callback to the reactor
//!
//! This method adds an IPC channel and associated callback method with the appropriate
//! signature to the reactor.
//!
//! \param channel IpcChannel to add to reactor
//! \param callback function reference to callback method

void IpcReactor::register_channel(IpcChannel& channel, ReactorCallback callback)
{
  // Add channel to channel map
  channels_[&(channel.socket_)] = callback;

  // Signal a rebuild is required
  needs_rebuild_ = true;
}

//! Removes an IPC chanel from the reactor
//!
//! This method removes an IPC channel and its associated callback method from the
//! reactor.
//!
//! \param channel IpcChannel to remove from the reactor

void IpcReactor::remove_channel(IpcChannel& channel)
{
  // Erase the channel from the map
  channels_.erase(&(channel.socket_));

  // Signal a rebuild is required
  needs_rebuild_ = true;
}

void IpcReactor::register_socket(int socket_fd, ReactorCallback callback)
{
  sockets_[socket_fd] = callback;

  needs_rebuild_ = true;
}

void IpcReactor::remove_socket(int socket_fd)
{
  sockets_.erase(socket_fd);

  needs_rebuild_ = true;
}

//! Adds a timer to the reactor
//!
//! This method adds a timer to the reactor. The periodic delay of the timer, the
//! number of times it should fire and the callback method to be called on firing
//! are specified.
//!
//! \param delay_ms periodic timer delay in milliseconds
//! \param times number of times the timer should fire (0=indefinite)
//! \param callback function reference to callback method
//! \return integer unique timer ID, which can be used by the caller to delete it subsequently

int IpcReactor::register_timer(size_t delay_ms, size_t times, TimerCallback callback)
{
  // Take lock while modifying timers_
  std::lock_guard<std::mutex> lock(mutex_);

  // Create a smart pointer to a new timer object
  std::shared_ptr<IpcReactorTimer> timer(new IpcReactorTimer(delay_ms, times, callback));

  // Add the timer to the timer map
  timers_[timer->get_id()] = timer;

  // Return the unique ID
  return timer->get_id();
}

//! Removes a timer from the reactor
//!
//! This method removes a timer from the reactor, based on the timer ID
//
//! \param timer_id integer unique timer ID that was returned by the add_timer() method
void IpcReactor::remove_timer(int timer_id)
{
  // Take lock while modifying timers_
  std::lock_guard<std::mutex> lock(mutex_);
  timers_.erase(timer_id);
}

//! Runs the reactor polling loop
//!
//! This method runs the reactor polling loop, handling any callbacks to
//! registered channels and timers. The loop runs indefinitely until an
//! error occurs or the stop() method is called. The loop uses a tickless
//! timeout based on the currently registered timers.
//!
//! \return integer return code, 0 = OK, -1 = error

int IpcReactor::run(void)
{
  int rc = 0;
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);

  // Loop until the terminate flag is set
  while (!terminate_reactor_)
  {
    // If the poll items list needs rebuilding, do it now
    if (needs_rebuild_)
    {
      rebuild_pollitems();
    }

    // If there are no channels to poll and no timers currently active, break out of the
    // reactor loop cleanly
    if ((pollsize_ == 0) && (timers_.size() == 0))
    {
      rc = 0;
      break;
    }

    try
    {

      // Poll the registered channels, using the tickless timeout based
      // on the next pending timer
      int pollrc = zmq::poll(pollitems_, pollsize_, calculate_timeout());

      if (pollrc > 0)
      {
        // If there were any channels ready to read, execute their callbacks
        for (size_t item = 0; item < pollsize_; ++item)
        {
          // TODO handle error flag on pollitems
          if (pollitems_[item].revents & ZMQ_POLLIN)
          {
            callbacks_[item]();
          }
        }
      }
      else if (pollrc == 0)
      {
        // Poll timed out, do nothing as we handle timers firing unconditionally below
      }
      else
      {
        // An error occurred, terminate the reactor loop
        rc = -1;
        terminate_reactor_ = true;
      }

      // Take lock while accessing timers_
      lock.lock();
      // Handle any timers that have now fired, calling their callbacks. Erase any timers
      // that have now expired
      TimerMap::iterator it = timers_.begin();
      while (it != timers_.end())
      {
        if ((it->second)->has_fired())
        {
          (it->second)->do_callback();
        }
        if ((it->second)->has_expired())
        {
          timers_.erase(it++);
        }
        else
        {
          ++it;
        }
      }
      lock.unlock();
    }
    catch ( zmq::error_t& e)
    {
      // If the exception was thrown with errno EINTR, i.e. interrupted system call, this is
      // because we have installed a custom signal handler, so terminate the reactor gracefully
      if (e.num() == EINTR) {
        rc = -1;
        terminate_reactor_ = true;
      }
        // Otherwise propogate the exception upwards
      else {
        std::stringstream ss;
        ss << "IpcReactor error while polling: " << e.what();
        throw IpcReactorException(ss.str());
      }
    }
  }

  return rc;
}

//! Signals that the reactor polling loop should stop gracefully
//!
//! This method is used to signal that the reactor polling loop should stop gracefully.

void IpcReactor::stop(void)
{
  terminate_reactor_ = true;
}

//! Rebuilds the internal list of polling item
//!
//! This private method rebuilds the internal list of items to poll in the reactor
//! loop. This is done when the rebuild_flag is set, i.e. after adding or
//! deleting channels.

void IpcReactor::rebuild_pollitems(void)
{

  // If the existing pollitems array is valid, delete it
  if (pollitems_) {
    delete[] pollitems_;
    pollitems_ = 0;
  }

  // If the existing callback array is valid, delete it
  if (callbacks_) {
    delete[] callbacks_;
    callbacks_= 0;
  }

  // Set the number of items to poll to the number of channels registered
  pollsize_ = channels_.size() + sockets_.size();

  // If the numer of items to poll is non-zero, craete new pollitems and callback
  // arrays and populate them

  if (pollsize_) {

    pollitems_ = new zmq::pollitem_t[pollsize_];
    callbacks_ = new ReactorCallback[pollsize_];

    // Iterate over the channel map and build the pollitems and callback arrays. These have
    // a one-to-one correspondence, allowing the reactor loop to easy associate an active
    // channel socket with the appropriate callback
    unsigned int item = 0;
    for (ChannelMap::iterator it = channels_.begin(); it != channels_.end(); ++item, ++it)
    {
      zmq::pollitem_t pollitem = {*(it->first), 0, ZMQ_POLLIN, 0};
      pollitems_[item] = pollitem;
      callbacks_[item] = it->second;
    }

    for (SocketMap::iterator it = sockets_.begin(); it != sockets_.end(); ++item, ++it)
    {
      zmq::pollitem_t pollitem = {0, it->first, ZMQ_POLLIN, 0};
      pollitems_[item] = pollitem;
      callbacks_[item] = it->second;
    }
  }
}

//! Calculates the next poll timeout based on the tickless pattern
//!
//! This private method calculates the next reactor poll timeout based on the
//! 'tickless' idiom in the CZMQ zloop implementation. The timeout is set
//! to match the next timer due to fire.
//!
//! \return long timeout value in milliseconds

long IpcReactor::calculate_timeout(void)
{
  // Take lock while accessing timers_
  std::lock_guard<std::mutex> lock(mutex_);

  // Calculate shortest timeout up to one hour (!!), looping through
  // current timers to see which fires first
  TimeMs tickless = IpcReactorTimer::clock_mono_ms() + (1000 * 3600);
  for (TimerMap::iterator it = timers_.begin(); it != timers_.end(); ++it)
  {
    if (tickless > (it->second)->when())
    {
      tickless = (it->second)->when();
    }
  }

  // Calculate current timeout based on that, set to zero (don't wait) if
  // there is no timers pending
  long timeout = (long)(tickless - IpcReactorTimer::clock_mono_ms());
  if (timeout < 0)
  {
    timeout = 0;
  }

  return timeout;
}
