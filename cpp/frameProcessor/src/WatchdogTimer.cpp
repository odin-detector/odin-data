/*
 * WatchdogTimer.cpp
 *
 *  Created on: 10 Mar 2020
 *      Author: Gary Yendell
 */

#include "WatchdogTimer.h"

#include "DebugLevelLogger.h"
#include "logging.h"

#define WARNING_DURATION_FRACTION 0.1 // Fraction of error duration that will cause a warning to be logged

namespace FrameProcessor {

WatchdogTimer::WatchdogTimer(const std::function<void(const std::string&)>& timeout_callback) :
    timer_id_(0),
    is_valid_id_(false),
    ticks_(0),
    timeout_callback_(timeout_callback)
{
    worker_thread_ = std::thread { [this]() { this->run(); } };

    LOG4CXX_TRACE(Logger::getLogger("FP.WatchdogTimer"), "WatchdogTimer constructor");
}

WatchdogTimer::~WatchdogTimer()
{
    worker_thread_running_.store(false);
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
void WatchdogTimer::start_timer(std::string&& function_name, unsigned int watchdog_timeout_ms)
{
    gettime(&start_time_, true);
    timeout_ = watchdog_timeout_ms;
    this->function_name_ = std::move(function_name);

    // Register timer to call timeout callback in watchdog_timeout milliseconds once
    if (watchdog_timeout_ms > 0) {
        LOG4CXX_DEBUG_LEVEL(
            1, Logger::getLogger("FP.WatchdogTimer"),
            "" << this->function_name_ << " | Registering " << watchdog_timeout_ms << "ms watchdog timer"
        );
        timer_id_ = reactor_.register_timer(
            watchdog_timeout_ms, 1,
            // Bind member function to this instance with function_name argument
            [this]() { this->call_timeout_callback(this->function_name_); }
        );
        is_valid_id_ = true;
    }
}

/**
 * Disable the watchdog, calculate the duration and then log and return
 *
 * To be called after a function returns
 *
 * \return - Duration in microseconds
 */
unsigned int WatchdogTimer::finish_timer()
{
    if (is_valid_id_) {
        reactor_.remove_timer(timer_id_);
    }
    is_valid_id_ = false;

    struct timespec now;
    gettime(&now, true);
    double duration = elapsed_us(start_time_, now);
    if (timeout_ > 0 && duration / 1000 > timeout_ * WARNING_DURATION_FRACTION) {
        LOG4CXX_WARN(Logger::getLogger("FP.WatchdogTimer"), function_name_ << " | Call took " << duration << "us");
    } else {
        LOG4CXX_DEBUG_LEVEL(
            1, Logger::getLogger("FP.WatchdogTimer"), function_name_ << " | Call took " << duration << "us"
        );
    }

    return duration;
}

/**
 * Function run by the worker_thread_
 *
 * This will register a heartbeat timer and then run the IpcReactor
 */
void WatchdogTimer::run()
{
    OdinData::configure_logging_mdc(OdinData::app_path.c_str());

    // Register a repeating timer to keep the reactor alive and check for shutdown every millisecond
    reactor_.register_timer(1, 0, [this]() { this->heartbeat(); });
    reactor_.run();
}

/**
 * Call the timeout_callback_ with an error message
 */
void WatchdogTimer::call_timeout_callback(const std::string& function_name) const
{
    timeout_callback_(function_name + " | Watchdog timed out");
}

/**
 * Report the reactor is still alive and stop when flag set
 */
void WatchdogTimer::heartbeat()
{
    if (!worker_thread_running_.load(std::memory_order::memory_order_acquire)) {
        LOG4CXX_DEBUG_LEVEL(1, Logger::getLogger("FP.WatchdogTimer"), "Terminating watchdog reactor");
        reactor_.stop();
    }
    ++ticks_;
    ticks_ = (ticks_ >= 1000) ? 0 : ticks_;
    if (ticks_ == 0)
        LOG4CXX_DEBUG_LEVEL(1, Logger::getLogger("FP.WatchdogTimer"), "Reactor running");
}

} /* namespace FrameProcessor */
