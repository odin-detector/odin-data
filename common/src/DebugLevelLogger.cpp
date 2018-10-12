#include "DebugLevelLogger.h"

DebugLevel debug_level = 0;

void set_debug_level(DebugLevel level)
{
  debug_level = level;
}
