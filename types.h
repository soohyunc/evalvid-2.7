#ifndef TRACE_TYPES
#define TRACE_TYPES

#if defined(_WIN32)
  #include <winsock2.h>
  typedef unsigned __int64 u64;
  typedef SOCKET sock_t;
#elif defined(__linux__)
    #include <gpac/setup.h>
  typedef int sock_t;
  // typedef unsigned long long u64;
#else
  #include <stdint.h>
  typedef int sock_t;
  typedef uint64_t u64;
#endif

enum prot { INV, UDP, TCP };

#endif
