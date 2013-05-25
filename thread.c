#include <stdio.h>

#include "error.h"
#include "thread.h"

threadid_t createthread(thread_t *t, fpthread_t f, void *context)
{
#if defined(_WIN32)
  if (0 == CreateThread(0, 0, f, context, 0, &t->id)) goto CT;
#elif defined(__linux__) || defined(__APPLE__)
  if (0 != pthread_create(&t->id, 0, f, context)) goto CT;
#endif
  t->running = 1;
  return t->id;

CT: seterror(err_CT);
    return 0;
}

void stopthread(thread_t *t)
{
  t->running = 0;
}
