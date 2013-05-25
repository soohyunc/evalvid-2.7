#include <stdio.h>

#include "error.h"
#include "lock.h"

int createlock(lock_t *l)
{
#if defined(_WIN32)
  InitializeCriticalSection(l);
#elif defined(__linux__) || defined(__APPLE__)
  if (0 != pthread_mutex_init(l, 0)) goto CT;
#endif
    return 1;
#if defined(__linux__) || defined(__APPLE__)
CT: seterror(err_CL);
    return 0;
#endif
}

void deletelock(lock_t *l)
{
#if defined(_WIN32)
  DeleteCriticalSection(l);
#elif defined(__linux__) || defined(__APPLE__)
  pthread_mutex_destroy(l);
#endif
}

int lock(lock_t *l)
{
#if defined(_WIN32)
  EnterCriticalSection(l);
#elif defined(__linux__) || defined(__APPLE__)
  if (0 != pthread_mutex_lock(l)) goto LT;
#endif
  return 1;
#if defined(__linux__) || defined(__APPLE__)
LT: seterror(err_LT);
    return 0;
#endif
}

int unlock(lock_t *l)
{
#if defined(_WIN32)
  LeaveCriticalSection(l);
#elif defined(__linux__) || defined(__APPLE__)
  if (0 != pthread_mutex_unlock(l)) goto LT;
#endif
  return 1;
#if defined(__linux__) || defined(__APPLE__)
LT: seterror(err_LT);
    return 0;
#endif
}
