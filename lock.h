#ifndef TRACE_LOCK
#define TRACE_LOCK

#if defined(_WIN32)
  #include <windows.h>
  typedef CRITICAL_SECTION lock_t;
#elif defined(__linux__) || defined(__APPLE__)
  #include <pthread.h>
  typedef pthread_mutex_t lock_t;
#endif

int createlock(lock_t *);
void deletelock(lock_t *);
int lock(lock_t *);
int unlock(lock_t *);

#endif
