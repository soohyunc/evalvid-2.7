
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "read.h"
#include "random.h"

static data_t D;
static const char FMT_T[] = "%10.6f SRC > DST (%d): udp %u (ttl 64, id %u, lost %d)\n"; 

int GenRD_Std(char *fn, dump_t *pP, unsigned l, int model, unsigned BER, unsigned PoB)
{
#define R (RETR.N[type] - retry.N[type])

  FILE *f;
  const unsigned HEAD = 8 * (20 + 8 + 16); /* IP + UDP + RTP */
  const unsigned OVRH = 1200; /* RTS, CTS, HDR, waitACK, transmission overhead [micro sec] */
  const unsigned BPS[]  = {11000000, 5500000, 2000000, 1000000};

  union {
    unsigned N[3];
    unsigned I, P, B;
  } RETR = {0, 0, 0}, RATE = {2, 2, 2}, retry;

  unsigned i, j, n, N = 0, type, ber;
  double bm = 0, t;

  int (*biterr)(int);

  switch (model) {
    case 1 : init_GE(BER, 50, 10000);
      biterr = GilbertElliot; break;
    case 0 :
    default: biterr = Gauss; break;
  }

  i = 0;
  while (i < l) {
    t = 0;
    type = pP[i].type == 'H' || pP[i].type == 'I' ? 0 : pP[i].type == 'P' ? 1 : 2;
    retry = RETR;

Retry:
    ber = 2 * BER / (1 << (3 - RATE.N[type]));

    for (n = 0, j = 0; j < 8 * pP[i].size; j++) if (biterr(ber)) n++;

    bm += n / (8. * pP[i].size);
    t += (HEAD + 8. * pP[i].size) / BPS[RATE.N[type]] + OVRH / 1000000.;
    N++;

    if (n) {
      if (retry.N[type]) {
        --retry.N[type];
        goto Retry;
      }
    } else pP[i].lost = 0;

    pP[i].retry = R;

    if (t > PoB) t = PoB, pP[i].lost = 1;

    pP[i].t2 = (i ? MAX(pP[i-1].t2, pP[i].t1) : pP[i].t1) + t;

    i++;
  }

  printf("%g\n", bm / N);

  if ((f = fopen(fn, "w")) != 0) {
    for (i = 0; i < l; i++) fprintf(f, FMT_T, pP[i].t2, pP[i].retry, pP[i].size, pP[i].id, pP[i].lost);
    fclose(f);
  } else {
    fprintf(stderr, "error opening %s\n", fn);
    return 0;
  }

  return 1;
#undef R
}

int main(int cn, char **cl)
{
  unsigned BER, PoB, model;

  if (cn < 5) {
    puts("usage: eg <sd> <rd> <st> <model> [BER] [PoB]");
    puts("  <sd>     tcpdump sender");
    puts("  <rd>     tcpdump receiver (will be generated)");
    puts("  <st>     tracefile sender");
    puts("  <model>  AWGN or GE");
    puts("  [BER]    reciprocal Bit Error Rate (1000 -> 0.001)");
    puts("  [PoB]    Play-out buffer [ms]");
    return 0;
  }

  BER = (cn > 5) ? strtoul(cl[5], 0, 10) : 0;
  if (BER < 10 || BER > 10000000) {
    BER = 10000;
    fprintf(stderr, "BER set to %g.\n", 1. / BER);
  }

  if (!strcmp(cl[4], "AWGN")) model = 0; else
  if (!strcmp(cl[4], "GE"))   model = 1;
  else {
    fprintf(stderr, "unknown error model");
    return EXIT_FAILURE;
  }

  PoB = (cn > 6) ? strtoul(cl[6], 0, 10) : 0;
  if (PoB < 10 || PoB > 10000) {
    PoB = 250;
    fprintf(stderr, "Play-out buffer set to %u ms.\n", PoB);
  }

  init_rand();

  if (!ReadDump(cl, &D, 2, 0)) return EXIT_FAILURE;
  if (!GenRD_Std(cl[2], D.P, D.nP, model, BER, PoB)) return EXIT_FAILURE;

  free(D.P);
  free(D.F);

  return 0;
}
