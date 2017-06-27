/*
 * IpcReactor.h - Frame Receiver Inter-Process Communication Channel multiplexed reactor class
 *
 * This class implements a reactor pattern, inspired by the zloop implementation in the CZMQ
 * high-level bindings, to allow multiple IPC channels to be multiplexed into an event loop.
 * A callback function for a channel can be added to the reactor, which will be called when
 * there is data present on the channel to receive. The reactor does not read the data; that
 * is the responsibility of the handler. Periodic timers can also be added to the reactor to
 * track e.g. timeouts or execute actions. Timers can expire after a certain number of firings
 * or run indefinitely. The reactor polls all registered channels with a 'tickless' event
 * loop to minimise load.
 *
 *  Created on: Feb 16, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef IPCREACTOR_H_
#define IPCREACTOR_H_

#include <iostream>
#include <sstream>
#include <map>
#include <time.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include "zmq/zmq.hpp"

#include "IpcChannel.h"
#include "OdinDataException.h"

namespace OdinData
{

//! IpcReactorException - custom exception class implementing "what" for error string
class IpcReactorException : public OdinDataException {
public:
  IpcReactorException(const std::string what) : OdinDataException(what) { }
};

//! Function signature for timer callback methods
typedef boost::function<void()> TimerCallback;

//! Reactor millisecond time type
typedef int64_t TimeMs;

//! IpcReactorTimer - timer objects for use in the IpcReactor class
class IpcReactorTimer
{
public:

  IpcReactorTimer(size_t delay_ms, size_t times, TimerCallback callback);
  ~IpcReactorTimer();

  //! get_id - returns the unique ID of the timer instance
  int get_id(void);

  //! Executes the registered callback method of the timer
  void do_callback(void);

  //! Indicates if the timer has fired (e.g. is due for handling)
  bool has_fired(void);

  //! Indicates if the timer has expired, i.e. reached its maximum times fired
  bool has_expired(void);

  //! Indicates when (in absolute monotonic time) the timer is due to fire
  TimeMs when(void);

  //! Returns the current monotonic clock time in milliseconds (static method)
  static TimeMs clock_mono_ms(void);

private:

  // Private member variables
  int timer_id_;             //!< Unique ID for the timer
  size_t delay_ms_;          //!< Timer delay in milliseconds
  size_t times_;             //!< Number of times the timer has left to fire
  TimerCallback callback_;   //!< Callback method to be called when the timer fires
  TimeMs when_;              //!< Time when the timer is next due to fire
  bool expired_;             //!< Indicates timers has expired

  static int last_timer_id_; //!< Class variable of last timer ID assigned
};

//! Function signature for reactor callback methods
typedef boost::function<void()> ReactorCallback;

//! Pointer to underlying ZMQ socket of a channel
typedef zmq::socket_t* SocketPtr;

//! Internal map to associate channel socket with a callback method
typedef std::map<SocketPtr, ReactorCallback> ChannelMap;

typedef std::map<int, ReactorCallback> SocketMap;

//! Internal map to associate timer ID with a timer
typedef std::map<int, boost::shared_ptr<IpcReactorTimer> > TimerMap;

class IpcReactor
{
public:

  IpcReactor(void);
  ~IpcReactor();

  //! Adds an IPC channel and associated callback to the reactor
  void register_channel(IpcChannel& channel, ReactorCallback callback);

  //! Removes an IPC chanel from the reactor
  void remove_channel(IpcChannel& channel);

  void register_socket(int socket_fd, ReactorCallback callback);

  void remove_socket(int socket_fd);

  //! Adds a timer to the reactor
  int register_timer(size_t delay_ms, size_t times, ReactorCallback callback);

  //! Removes a timer from the reactor
  void remove_timer(int timer_id);

  //! Runs the reactor polling loop
  int run(void);

  //! Signals that the reactor polling loop should stop gracefully
  void stop(void);

private:

  //! Rebuilds the internal list of polling items
  void rebuild_pollitems(void);

  //! Calculates the next poll timeout based on the tickless pattern
  long calculate_timeout(void);

  // Private member variables

  bool terminate_reactor_;         //!< Indicates that the reactor loop should terminate
  ChannelMap channels_;            //!< Map of channels associated with the reactor
  SocketMap  sockets_;
  TimerMap   timers_;              //!< Map of timers associated with the reactor
  zmq::pollitem_t* pollitems_;     //!< Ptr to array of pollitems to use in poll call
  ReactorCallback* callbacks_;     //!< Ptr to matched array of callbacks
  std::size_t      pollsize_;      //!< Number if active items to poll
  bool             needs_rebuild_; //!< Indicates that the poll item list needs rebuilding
};

} // namespace OdinData
#endif /* IPCREACTOR_H_ */
