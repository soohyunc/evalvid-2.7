#ifndef TRACE_DEF
#define TRACE_DEF

#ifndef L
  #define L 152064 /* (352 * 288 * 1.5) */
#elif
  #error "L defined elsewhere"
#endif

#ifndef MIN
  #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct {
  unsigned id, size, retry, rest, segm;
  double t1, t2, j1, j2, d1, d2;
  char type, lost, signal, noise;
} dump_t;

typedef struct {
  dump_t *P, *F;
  unsigned long nP, nF;
} data_t;

typedef struct {
  unsigned long lH, lI, lP, lB, la, lA, nH, nI, nP, nB, na, nA;
} loss_t;

typedef enum mode {
  INVALID,
  PACKET   = 1,
  FRAME    = 2,
  FILL     = 4,
  TRUNC    = 8,
  GEN      = 16,
  RAW      = 32,
  AUDIO    = 64,
  COMPLETE = 128,
  ASYNC    = 256
} MODE;

#endif
