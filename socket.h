#ifndef TRACE_SOCKET
#define TRACE_SOCKET

#include "types.h"
#include "rtp.h"

int setdest(char *, unsigned short, enum prot, int);
int sendbuf(unsigned char *, unsigned, enum ptype);
void cleanup(void);

#endif
