#include "pch.h"
#include "hi_res_time.h"

static double g_tickInterval = 1.0;
static double g_lastGetHighResTime = 0.0;
static double g_timeShift = 0.0;

// *** InitialiseHighResTime
void InitialiseHighResTime()
{
  // Start be getting the frequency the Performance Counter uses.
  // We need to use the Performance Counter to calibrate the Pentium
  // Counter, if we are going to use it.
  LARGE_INTEGER count;
  QueryPerformanceFrequency(&count);
  g_tickInterval = 1.0 / static_cast<double>(count.QuadPart);
}

inline double GetLowLevelTime()
{
  LARGE_INTEGER count;

  static LARGE_INTEGER startTime;
  static bool initted = false;

  if (!initted)
  {
    QueryPerformanceCounter(&startTime);
    initted = true;
  }

  QueryPerformanceCounter(&count);
  return static_cast<double>(count.QuadPart - startTime.QuadPart) * g_tickInterval;
}

// *** GetHighResTime
double GetHighResTime()
{
  double timeNow = GetLowLevelTime();
  timeNow -= g_timeShift;

  g_lastGetHighResTime = timeNow;
  return timeNow;
}
