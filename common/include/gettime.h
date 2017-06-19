/*
 * gettime.h
 *
 *  Created on: 23 Oct 2015
 *      Author: ulrik
 */

#ifndef INCLUDE_GETTIME_H_
#define INCLUDE_GETTIME_H_

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

// Our own version of clock_gettime or clock_get_time which works for both Linux and Mac OSX
static inline void gettime(struct timespec *ts, bool monotonic=false) {

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
clock_serv_t cclock;
clock_id_t clockid;
if (monotonic) clockid = SYSTEM_CLOCK;
else clockid = CALENDAR_CLOCK;
mach_timespec_t mts;
host_get_clock_service(mach_host_self(), clockid, &cclock);
clock_get_time(cclock, &mts);
mach_port_deallocate(mach_task_self(), cclock);
ts->tv_sec = mts.tv_sec;
ts->tv_nsec = mts.tv_nsec;
#else
clockid_t clockid;
if (monotonic) clockid = CLOCK_MONOTONIC;
else clockid = CLOCK_REALTIME;
clock_gettime(clockid, ts);
#endif

}
#endif /* INCLUDE_GETTIME_H_ */
