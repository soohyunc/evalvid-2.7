#ifndef TRACE_STAT
#define TRACE_STAT

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

void CalcTime(dump_t *, unsigned, char *);
void CalcLoss(dump_t *, unsigned, loss_t *, char *);
void CalcDist(dump_t *, unsigned, loss_t *);
void CalcJitter(dump_t *, unsigned);
void PoBLoss(dump_t *, unsigned, unsigned, MODE);
void OutJitter(dump_t *, unsigned, char *);
void CalcSRate(dump_t *, unsigned, double, char *);
void CalcRRate(dump_t *, unsigned, double, char *);

void Hist(double *, unsigned, double, double, unsigned);

#ifdef __cplusplus
}
#endif

#endif
