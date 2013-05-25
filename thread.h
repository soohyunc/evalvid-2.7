#ifndef TRACE_THREAD
#define TRACE_THREAD

#if defined(_WIN32)
  #include <windows.h>
  #define ret_t DWORD WINAPI /* WINAPI can't be typedefed */
  typedef LPTHREAD_START_ROUTINE fpthread_t;
  typedef DWORD threadid_t;
#elif defined(__linux__) || defined(__APPLE__)
  #include <pthread.h>
  typedef void *ret_t;
  typedef void *(*fpthread_t)(void *);
  typedef pthread_t threadid_t;
#endif

typedef struct thread {
  threadid_t id;
  volatile int running;
} thread_t;

threadid_t createthread(thread_t *, fpthread_t, void *);
void stopthread(thread_t *);

#endif
