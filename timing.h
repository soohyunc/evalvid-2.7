#ifndef TRACE_TIMING
#define TRACE_TIMING

#include "types.h"

#if defined(_WIN32)
  #include <windows.h>
  #define INLINE __inline
  #define SLEEP(x) Sleep(x)
#elif defined(__linux__)
  #include <string.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/resource.h>
  #include <sys/time.h>
  #define INLINE __inline__
  #define SLEEP(x) usleep(1000 * (x) - 1)
#elif defined (__APPLE__)
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #include <sys/resource.h>
  #include <sys/time.h>
  #define INLINE __inline__
  #define SLEEP(x) usleep(1000 * (x) - 1)
#else
  #define INLINE
  #define SLEEP(x)
#endif

u64 get_hpc(void);
int cpufreq(void);
void measfreq(void);
void get_cpu_freq(void);
u64 freq(void);
void starttimer(void);
double curtime(void);

#endif
