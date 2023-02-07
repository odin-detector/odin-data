/*
 * WatchdogTimer.cpp
 *
 *  Created on: 10 Mar 2020
 *      Author: Gary Yendell
 */

#include "WatchdogTimer.h"

#include "logging.h"
#include "DebugLevelLogger.h"

#define WARNING_DURATION_FRACTION 0.1  // Fraction of error duration that will cause a warning to be logged

namespace FrameProcessor {

WatchdogTimer::WatchdogTimer(const boost::function<void(const std::string&)>& timeout_callback) :
        worker_thread_running_(false),
        worker_thread_(boost::bind(&WatchdogTimer::run, this)),
        timeout_callback_(timeout_callback)
{
  this->logger_ = Logger::getLogger("FP.WatchdogTimer");

  // Wait until worker thread is ready before returning
  while (!worker_thread_running_) {}

  LOG4CXX_TRACE(logger_, "WatchdogTimer constructor");
}

WatchdogTimer::~WatchdogTimer() {
  worker_thread_running_ = false;
  worker_thread_.join();
}

/**
 * Store the start time and schedule a deadline_timer to print an error
 *
 * To be called before a function call
 *
 * \param[in] function_name - Function name for log message
 * \param[in] watchdog_timeout_ms - Timeout for watchdog to log error message
 */
void WatchdogTimer::start_timer(const std::string& function_name, unsigned int watchdog_timeout_ms) {
  gettime(&start_time_, true);
  timeout_ = watchdog_timeout_ms;
  function_name_ = function_name;

  // Register timer to call timeout callback in watchdog_timeout milliseconds once
  if (watchdog_timeout_ms > 0) {
    LOG4CXX_DEBUG_LEVEL(1, logger_,
      "" << function_name << " | Registering " << watchdog_timeout_ms << "ms watchdog timer"
    );
    timer_id_ = reactor_.register_timer(
        watchdog_timeout_ms, 1,
        // Bind member function to this instance with function_name argument
        boost::bind(&WatchdogTimer::call_timeout_callback, this, function_name)
    );
  }
}

/**
 * Disable the watchdog, calculate the duration and then log and return
 *
 * To be called after a function returns
 *
 * \return - Duration in microseconds
 */
unsigned int WatchdogTimer::finish_timer() {
  reactor_.remove_timer(timer_id_);

  struct timespec now;
  gettime(&now, true);
  double duration = elapsed_us(start_time_, now);

  std::stringstream message;
  message << function_name_ << " | Call took " << duration << "us";
  if (timeout_ > 0 && duration / 1000 > timeout_ * WARNING_DURATION_FRACTION) {
    LOG4CXX_WARN(logger_, message.str());
  } else {
    LOG4CXX_DEBUG_LEVEL(1, logger_, message.str());
  }

  return duration;
}

/**
 * Function run by the worker_thread_
 *
 * This will register a heartbeat timer and then run the IpcReactor
 */
void WatchdogTimer::run() {
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());

  // We are ready - Let the constructor return
  worker_thread_running_ = true;

  // Register a repeating timer to keep the reactor alive and check for shutdown every millisecond
  reactor_.register_timer(1, 0, boost::bind(&WatchdogTimer::heartbeat, this));
  reactor_.run();
}

/**
 * Call the timeout_callback_ with an error message
 */
void WatchdogTimer::call_timeout_callback(const std::string& function_name) const {
  std::stringstream error_message;
  error_message << function_name << " | Watchdog timed out";
  timeout_callback_(error_message.str());
}

/**
 * Report the reactor is still alive and stop when flag set
 */
void WatchdogTimer::heartbeat() {
  if (!worker_thread_running_) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Terminating watchdog reactor");
    reactor_.stop();
  } else if (ticks_ >= 1000) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Reactor running");
    ticks_ = 0;
  } else {
    ticks_++;
  }
}

} /* namespace FrameProcessor */
