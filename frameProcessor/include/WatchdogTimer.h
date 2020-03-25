/*
 * WatchdogTimer.h
 *
 *  Created on: 10 Mar 2020
 *      Author: Gary Yendell
 */

#ifndef FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_
#define FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <IpcReactor.h>
#include <gettime.h>

#include <log4cxx/logger.h>
using namespace log4cxx;

namespace FrameProcessor {

class WatchdogTimer
{
public:
  WatchdogTimer(const boost::function<void(const std::string&)>& timeout_callback);
  ~WatchdogTimer();

  void start_timer(const std::string& function_name, unsigned int watchdog_timeout_ms);
  unsigned int finish_timer();

private:
  void run();
  void call_timeout_callback(const std::string& function_name) const;
  void heartbeat();

  /** Logger for logging */
  LoggerPtr logger_;
  /* Store for start time of watchdog */
  struct timespec start_time_;
  /* Timer watchdog thread */
  boost::thread worker_thread_;
  /** Flag to control start up timings of main thread and worker thread */
  bool worker_thread_running_;
  /** IpcReactor to use as a simple timer controller */
  OdinData::IpcReactor reactor_;
  /** Timeout of current timer in milliseconds */
  unsigned int timeout_;
  /** Name of function currently being timed */
  std::string function_name_;
  /** ID of current timer to cancel callback */
  int timer_id_;
  /** Counter to monitor number of ticks that have passed */
  int ticks_;
  /** Callback function to call when the timer expires */
  const boost::function<void(const std::string&)>& timeout_callback_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_ */
