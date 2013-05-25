
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "def.h"
#include "stat.h"

void CalcLoss(dump_t *pD, unsigned l, loss_t *pS, char *id)
{
  FILE *f;
  unsigned long i, w = 0;
  char s[32];

  for (i=0; i<l; i++)
    switch (pD[i].type) {
      case 'H': pS->nH++; if (pD[i].lost) pS->lH++; break;
      case 'I': pS->nI++; if (pD[i].lost) pS->lI++; break;
      case 'P': pS->nP++; if (pD[i].lost) pS->lP++; break;
      case 'B': pS->nB++; if (pD[i].lost) pS->lB++; break;
      case 'A': pS->na++; if (pD[i].lost) pS->la++; break;
      default : if (pD[i].lost) w++; break;
    }

  pS->nA = pS->nH + pS->nI + pS->nP + pS->nB;
  pS->lA = pS->lH + pS->lI + pS->lP + pS->lB;

  if (id) {
    sprintf(s, "loss_%s.txt", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "Could not open %s\n", s);
      return;
    }
    if (!w && !pS->na)
      fprintf(f, "%8.2f%8.2f%8.2f%8.2f\n",
        pS->nI || pS->nH ? 100. * (pS->lI + pS->lH) / (pS->nI + pS->nH) : 0,
        pS->nP ? 100. * pS->lP / pS->nP : 0,
        pS->nB ? 100. * pS->lB / pS->nB : 0,
        pS->nA ? 100. * pS->lA / pS->nA : 0);
    else if (pS->na)
      fprintf(f, "%8.2f\n",
        pS->na ? 100. * pS->la / pS->na : 0);
    else
      fprintf(f, "%8.2f\n", l ? 100. * w / l : 0);

    if (pS->na)
      printf("*** loss_%s.txt ***: percentage of lost [frames|packets]\n"
             "    column 1: A\n\n", id);
    else
      printf("*** loss_%s.txt ***: percentage of lost [frames|packets]\n"
             "    column 1: I (including H)\n"
             "    column 2: P\n"
             "    column 3: B\n"
             "    column 4: overall\n\n", id);
    fclose(f);
  }
}

void CalcJitter(dump_t *pD, unsigned l)
{
  unsigned  i, j;
  double tmp;

  for (i=0; i<l; i++)
    pD[i].d1 = pD[i].lost ? 0 : pD[i].t2 - pD[i].t1;

  for (i=1; i<l; i++) {
    pD[i].j1 = pD[i].t1 - pD[i-1].t1;
    pD[i].j2 = pD[i].lost || pD[i-1].lost ? 0 : pD[i].t2 - pD[i-1].t2;

    tmp = pD[i].j2;
    if (!tmp)
      if (!pD[i].lost) {
        for (j=i-1; j>0; j--)
          if (!pD[j].lost) {
            tmp = pD[i].t2 - pD[j].t2;
            break;
          }
      }/* else tmp = pD[i].j1;*/

    pD[i].d2 = tmp - pD[i].j1;
  }

  pD[0].j1 = 0;
  pD[0].j2 = 0;
}

void CalcRRate(dump_t *pD, unsigned l, double interval, char *id)
{
  FILE *f;
  unsigned long i;
  double t, t0 = 0, t1 = 0, t2 = 0, b, c = 0;
  char s[32];

  if (id) {
    sprintf(s, "rate_r_%s.txt", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "Could not open %s\n", s);
      return;
    }

    for (i=0; i<l; i++) if (!pD[i].lost) {
      t0 = pD[i].t2;
      break;
    }

    while (i < l) {
      while (pD[i].lost) i++;
      t = pD[i].t2;
      b = 0;
      while (i < l && pD[i].t2 <= t + interval) {
        if (!pD[i].lost) {
          b += pD[i].size;
          c += pD[i].size;
          t2 = pD[i].t2;
        }
        i++;
      }
      t1 += interval;
      fprintf(f, "%.3f\t%.1f\t%.1f\n",
        t - pD[0].t1,
        b ? b / (t2 - t) : 0,
        c ? c / (t2 - t0) : 0);
    }
    printf("*** rate_r_%s.txt ***: receiver rate\n"
           "    column 1: time [s]\n"
           "    column 2: momentary rate [bytes/s]\n"
           "    column 3: cumulative rate [bytes/s]\n\n", id);
    fclose(f);
  }
}

void CalcSRate(dump_t *pD, unsigned l, double interval, char *id)
{
  FILE *f;
  unsigned long i = 0;
  double t, t0 = 0, t1 = 0, t2 = 0, b, c = 0;
  char s[32];

  if (id) {
    sprintf(s, "rate_s_%s.txt", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "Could not open %s\n", s);
      return;
    }

    t0 = pD[i].t1;

    while (i < l) {
      t = pD[i].t1;
      b = 0;
      while (i < l && pD[i].t1 <= t + interval) {
        b += pD[i].size;
        c += pD[i].size;
        t2 = pD[i].t1;
        i++;
      }
      t1 += interval;
      fprintf(f, "%.3f\t%.1f\t%.1f\n",
        t - t0,
        b ? b / (t2 - t) : 0,
        c ? c / (t2 - t0) : 0);
    }
    printf("*** rate_s_%s.txt ***: sender rate\n"
           "    column 1: time [s]\n"
           "    column 2: momentary rate [bytes/s]\n"
           "    column 3: cumulative rate [bytes/s]\n\n", id);
    fclose(f);
  }
}

void OutJitter(dump_t *pD, unsigned l, char *id)
{
  FILE *f=0;
  unsigned i;
  double cjit = 0;
  char s[32];

  if (id) {
    sprintf(s, "delay_%s.txt", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "error opening %s\n", s);
      return;
    }
    for (i=0; i<l; i++) {
      cjit += pD[i].d2;
      fprintf(f, "%u\t%d\t%.6f\t%.6f\t%.6f\t%.6f\n", i, !!pD[i].lost, pD[i].d1, pD[i].j1, pD[i].j2, cjit);
    }
    printf("*** delay_%s.txt ***: jitter/delay statistics\n"
           "    column 1: [frame|packet] id\n"
           "    column 2: loss flag\n"
           "    column 3: end-to-end delay [s]\n"
           "    column 4: sender inter [frame|packet] lag [s]\n"
           "    column 5: receiver inter [frame|packet] lag [s]\n"
           "    column 6: cumulative jitter [s] [Hartanto et. al.]\n\n", id);
    fclose(f);
  }
}

void CalcHist(dump_t *F, unsigned l, unsigned max, unsigned width, char *id)
{
  FILE *f;
  char s[32];
  unsigned i, j, *hist = malloc(max / width + 1);

  if (strlen(id) > 25) id[25] = 0;

  for (i=0, hist[i] = 0; i<l; i++) {
    j = (unsigned) (.5 + 1000 * F[i].d1 / width);
    if (j > max / width) j = max / width;
    hist[j]++;
  }

  if (id) {
    sprintf(s, "jitt_%s", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "Could not open %s\n", s);
      return;
    }
    for (i=0; i<max/width; i++) if (hist[i]) fprintf(f, "%u\t%g\n", i * width, 100. * hist[i] / l);
    fclose(f);
  }

  free(hist);
}

void PoBLoss(dump_t *pD, unsigned l, unsigned PoB, MODE mode)
{
  double cjit = 0, tdrop = PoB / 1000.;
  unsigned i;

  for (i = !!(mode & ASYNC); i < l; i++)
    if (mode & ASYNC) {
      cjit += pD[i].d2;
      if (!pD[i].lost && cjit > tdrop) pD[i].lost = 1;
    } else {
      if (!pD[i].lost && pD[i].t2 - pD[i].t1 > tdrop) pD[i].lost = 1;
    }
}

void CalcTime(dump_t *P, unsigned l, char *id)
{
  FILE *f;
  char s[32];

  if (id) {
    sprintf(s, "time_%s", id);
    if ((f = fopen(s, "w")) == 0) {
      fprintf(stderr, "Could not open %s\n", s);
      return;
    }
  } else f = stdout;

  fprintf(f, "%.2f\n", P[l-1].t2 - P[0].t2);

  fclose(f);
}

void Hist(double *D, unsigned l, double min, double max, unsigned interv)
{
  int k;
  unsigned i, *hist, num = 0;
  double cdf = 0;

  if ((hist = calloc(interv, sizeof *hist)) == 0) {
    fprintf(stderr, "malloc error!");
    return;
  }

  for (i=0; i<l; i++) {
    k = (int) (.5 + (fabs(D[i]) - min) / (max - min) * interv);
    if (k < 0) k = 0;
    if ((unsigned)k >= interv) k = interv - 1;
    ++hist[k];
    ++num;
  }

  for (i=0; i<interv; i++) {
    printf("%f\t", i * (max - min) / interv + min);
    cdf += (double)hist[i] / num;
    printf("%.6f\t", hist[i] ? 100. * hist[i] / num : 0);
    printf("%.6f\n", cdf);
  }

  free(hist);
}
