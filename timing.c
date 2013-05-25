
#include <stdio.h>
#include <stdlib.h>

#include "timing.h"

/* private */

static u64 freq_;
static u64 start_;
static int meas_;
/* public */

INLINE u64 get_hpc()
{
#if defined(_WIN32)
  if (meas_) {
    __asm { rdtsc }
  } else {
    u64 c;
    QueryPerformanceCounter((LARGE_INTEGER *)&c);
    return c;
  }
#elif defined(__APPLE__) && defined(__POWERPC__)
  union {
    u64 ll;
    unsigned l[2];
  } v;
  __asm__ volatile ("mftbu %0 \n mftb %1" : "=r" (v.l[0]), "=r" (v.l[1]));
  return v.ll;
#elif defined(__APPLE__) || defined(__linux__)
  u64 r;
  struct timeval t;
  gettimeofday(&t, 0);
  r = 1000000 * t.tv_sec + t.tv_usec;
  return r;
#endif
}

void measfreq()
{
  int i, p;
  u64 start, d, x = 0;

#if defined(_WIN32)
  p = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#elif defined(__linux__) || defined(__APPLE__)
  p = getpriority(PRIO_PROCESS, 0);
  setpriority(PRIO_PROCESS, 0, -20);
#endif

  for (i = 0; i < 3; i++) {
    start = get_hpc();
    SLEEP(1000);
    d = get_hpc() - start;
    if (d > x) x = d;
    fputc('.', stderr);
  }
  freq_ = x;

#if defined(_WIN32)
  SetPriorityClass(GetCurrentProcess(), p);
#elif defined(__linux__) || defined(__APPLE__)
  setpriority(PRIO_PROCESS, 0, p);
#endif
}

int cpufreq()
{
#if defined(_WIN32)
  return !!QueryPerformanceFrequency((LARGE_INTEGER *)&freq_);
#elif defined(__APPLE__) && defined(__POWERPC__)
  int m[2] = { CTL_HW, HW_CPU_FREQ };
  unsigned f;
  size_t size = sizeof f;
  freq_ = sysctl(m, 2, &f, &size, 0, 0) ? 0 : f;
  return !!freq_;
#elif defined(__linux__) || defined(__APPLE__)
  return freq_ = 1000000;
#endif
}

void get_cpu_freq()
{
  double t1, t2;

  if (!cpufreq()) {
    fprintf(stderr, "Calibrating high performance counter");
    measfreq();
    t1 = curtime();
    SLEEP(1000);
    t2 = curtime();
    fputc('.', stderr);
    if (abs((int)(1000 * (t2 - t1) - 1000)) > 9) measfreq();
    fputc('\n', stderr);
    meas_ = 1;
  }
}

u64 freq()
{
  return freq_;
}

void starttimer()
{
  start_ = get_hpc();
}

double curtime()
{
  u64 t = get_hpc() - start_;
  return t / (double)freq_;
}
