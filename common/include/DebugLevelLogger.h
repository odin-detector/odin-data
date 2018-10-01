/*
 * DebugLevelLogger.h
 *
 *  Created on: Mar 3, 2015
 *      Author: tcn
 */

#ifndef INCLUDE_DEBUGLEVELLOGGER_H_
#define INCLUDE_DEBUGLEVELLOGGER_H_

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

typedef unsigned int DebugLevel;

extern DebugLevel debug_level;

#define LOG4CXX_DEBUG_LEVEL(level, logger, message) { \
  if (LOG4CXX_UNLIKELY(level <= debug_level)) {\
    LOG4CXX_DEBUG(logger, message); }}

#define IMPLEMENT_DEBUG_LEVEL \
  DebugLevel debug_level = 0; \
  void set_debug_level(DebugLevel level) { debug_level = level; }

#endif /* INCLUDE_DEBUGLEVELLOGGER_H_ */
