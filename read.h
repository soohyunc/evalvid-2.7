#ifndef TRACE_READ
#define TRACE_READ

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

int ReadDump(char **, data_t *, MODE, int);
int GetNumB(dump_t *, unsigned);
int ReOrder(data_t *, int);
unsigned long MaxPackSize(dump_t *, unsigned);

#ifdef __cplusplus
}
#endif

#endif
