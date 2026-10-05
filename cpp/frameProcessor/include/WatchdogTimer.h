/*
 * WatchdogTimer.h
 *
 *  Created on: 10 Mar 2020
 *      Author: Gary Yendell
 */

#ifndef FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_
#define FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_

#include <IpcReactor.h>

#include <atomic>
#include <gettime.h>
#include <string>
#include <thread>

#include <log4cxx/logger.h>
using namespace log4cxx;

namespace FrameProcessor {

class WatchdogTimer {
public:
    WatchdogTimer(const std::function<void(const std::string&)>& timeout_callback);
    ~WatchdogTimer();

    void start_timer(const std::string& function_name, unsigned int watchdog_timeout_ms);
    unsigned int finish_timer();

private:
    void run();
    void call_timeout_callback(const std::string& function_name) const;
    void heartbeat();

    /* Store for start time of watchdog */
    struct timespec start_time_;
    /* Timer watchdog thread */
    std::thread worker_thread_;
    /**indicator to stop worker thread */
    std::atomic<bool> worker_thread_running_;
    /** IpcReactor to use as a simple timer controller */
    OdinData::IpcReactor reactor_;
    /** Timeout of current timer in milliseconds */
    unsigned int timeout_;
    /** Name of function currently being timed */
    std::string function_name_;
    /** ID of current timer to cancel callback */
    int timer_id_;
    /** Whether timer_id_ is valid */
    bool is_valid_id_;
    /** Counter to monitor number of ticks that have passed */
    int ticks_;
    /** Callback function to call when the timer expires */
    const std::function<void(const std::string&)>& timeout_callback_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_WATCHDOGTIMER_H_ */
