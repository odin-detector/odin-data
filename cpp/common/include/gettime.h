/*
 * gettime.h
 *
 *  Created on: 23 Oct 2015
 *      Author: ulrik
 */

#ifndef INCLUDE_GETTIME_H_
#define INCLUDE_GETTIME_H_

#include <ctime>
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

/** Calculate and return an elapsed time in microseconds.
 *
 * This method calculates and returns an elapsed time in microseconds based on the start and
 * end timespec structs passed as arguments.
 *
 * \param[in] start - start time in timespec struct format
 * \param[in] end - end time in timespec struct format
 * \return elapsed time between start and end in microseconds
 */
static inline unsigned int elapsed_us(const struct timespec& start, const struct timespec& end)
{
  double start_ns = ((double) start.tv_sec * 1000000000) + start.tv_nsec;
  double end_ns = ((double) end.tv_sec * 1000000000) + end.tv_nsec;

  return (unsigned int)((end_ns - start_ns) / 1000);
}

#endif /* INCLUDE_GETTIME_H_ */
