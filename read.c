
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "def.h"
#include "misc.h"
#include "read.h"

static const char FMT_T[] = "%*d %c %d%d%*[^\n]\n";

int ReadDump(char *fl[], data_t *D, MODE mode, int off)
{
  static dump_t null;
  FILE *f = 0;
  double t = 0, tmax;
  unsigned long i, ti, line, id = 0, lost;
  char buf[0x1000], *p = buf, *e, j, c;
  int l = 0, h, m, len;
  size_t num = 0;
  void *tmp;

  if ((f = fopen(fl[1], "r")) == 0) {
    fprintf(stderr, "error opening %s\n", fl[1]);
    return 0;
  }

  line = 0;
  while (c = 0, line++, !feof(f) && !ferror(f)) {
    if (!fgets(buf, sizeof buf, f) || feof(f) || ferror(f)) break;
    len = strlen(buf);
    if (len < 3 || *buf == '\n') continue;
    if (buf[len - 2] == ')') if (!fgets(buf + len - 1, sizeof buf - len, f) || feof(f) || ferror(f)) break;

    if (!(t = strtod(buf, &e)) && e == buf) { c = 1; goto e1; }

    if (!(p = skips(buf, " id ", 1)))
      if (!(p = skips(buf, "frag ", 1)))
        if (!(p = skips(buf, "udp/rtp", 1))) { c = 1; goto e1; }
        else {
          if (!(l = strtoul(p, &e, 10)) && e == p) { c = 1; goto e1; }
          l += 12; /* RTP Header */
          if (!(p = skips(p, " ", 4))) { c = 1; goto e1; }
          if (!(id = strtoul(p, &e, 10)) && e == p) { c = 1; }
          goto e1;
        }

    if (!(id = strtoul(p, &e, 10)) && e == p) { c = 1; goto e1; }

    if (!(p = skips(buf, " udp ", 1)))
      if (!(p = skips(buf, "UDP, length:", 1)))
        if (!(p = skips(buf, "UDP, length", 1))) { c = 1; goto e1; }

    if (!(l = strtoul(p, &e, 10)) && e == p) c = 1;

e1: if (c) {
      fprintf(stderr, "malformed input (%s, %lu)\n%s\n", fl[1], line, p);
      continue;
    }

    if (++D->nP > num) {
      num += 16384;
      if ((tmp = realloc(D->P, num * sizeof *D->P)) == 0) {
        fprintf(stderr, "realloc error\n");
        return 0;
      }
      D->P = tmp;
    }

    D->P[D->nP - 1] = null;
    D->P[D->nP - 1].id = id;
    D->P[D->nP - 1].t1 = t;
    D->P[D->nP - 1].size = l - off;
    D->P[D->nP - 1].lost = 1;
    D->P[D->nP - 1].type = ' ';
  }
  fclose(f);

  if (!(mode & GEN)) {
    if ((f = fopen(fl[2], "r")) == 0) {
      fprintf(stderr, "error opening %s\n", fl[2]);
      return 0;
    }

    line = m = 0;
    while (lost = 0, c = 0, line++, !feof(f) && !ferror(f)) {
      if (!fgets(buf, sizeof buf, f) || feof(f) || ferror(f)) break;
      len = strlen(buf);
      if (len < 3 || *buf == '\n') continue;
      if (buf[len - 2] == ')') if (!fgets(buf + len - 1, sizeof buf - len, f) || feof(f) || ferror(f)) break;

      if (!(t = strtod(buf, &e)) && e == buf) { c = 1; goto e2; }

      if (!(p = skips(buf, " id ", 1)))
        if (!(p = skips(buf, "frag ", 1)))
          if (!(p = skips(buf, "udp/rtp", 1))) { c = 1; goto e2; }
          else {
            if (!(p = skips(p, " ", 4))) { c = 1; goto e1; }
            if (!(id = strtoul(p, &e, 10)) && e == p) { c = 1; }
            goto e2;
          }

      if (!(id = strtoul(p, &e, 10)) && e == p) { c = 1; goto e2; }

  e2: if (c) {
        fprintf(stderr, "malformed input (%s, %lu)\n", fl[2], line);
        continue;
      }

      if (p = skips(buf, " lost ", 1)) lost = strtoul(p, 0, 10);

      for (i = m; i < D->nP; i++)
        if (id == D->P[i].id) {
          D->P[i].t2 = t;
          D->P[i].lost = !!lost;
          if (i > 100) m = i - 100;
          break;
        }
    }
    fclose(f);
  }

  if (!(mode & RAW)) {
    if ((f = fopen(fl[3], "r")) == 0) {
      fprintf(stderr, "error opening %s\n", fl[3]);
      return 0;
    }

    i = 0;
    num = line = 0;
    while (c = 0, line++, !feof(f) && !ferror(f)) {
      if (3 != fscanf(f, FMT_T, &c, &m, &l)) {
        if (!feof(f)) {
          fprintf(stderr, "malformed input (%s, %lu)\n", fl[3], line);
          continue;
        } else break;
      }

      if (i >= D->nP) {
        fprintf(stderr, "%s incomplete\n", fl[1]);
        return 0;
      }

      ti = i;
      tmax = D->P[i].lost ? 0 : D->P[i].t2;
      for (j = 0, h = 0; h < l; h++) {
        if (!j) j = mode & FRAME ? D->P[i].lost && (h == 0) : D->P[i].lost;
        if (!D->P[i].lost && D->P[i].t2 > tmax) tmax = D->P[i].t2;
        D->P[i++].type = c;
      }

      if (++D->nF > num) {
        num += 16384;
        if ((tmp = realloc(D->F, num * sizeof *D->F)) == 0) {
          fprintf(stderr, "realloc error\n");
          return 0;
        }
        D->F = tmp;
      }

      D->F[D->nF - 1] = null;
      D->F[D->nF - 1].size = m;
      D->F[D->nF - 1].lost = j;
      D->F[D->nF - 1].type = c;
      D->F[D->nF - 1].segm = l;
      D->F[D->nF - 1].t1 = mode & FRAME ? D->P[ti].t1 : D->P[i - 1].t1;
      D->F[D->nF - 1].t2 = tmax;
    }
    fclose(f);
  }

  return 1;
}

int GetNumB(dump_t *F, unsigned l)
{
  int numB = 0;
  unsigned i = 0;

  while (i < l) {
    if (F[i].type == 'B') break;
    i++;
  }

  while (i < l && F[i].type == 'B') {
    numB++;
    i++;
  }

  return numB;
}

int ReOrder(data_t *D, int nB)
{
  unsigned long i;
  dump_t *pF = malloc((D->nF - 1) * sizeof *pF);

  if (pF == 0) return 0;

  for (i=1; i<D->nF; i++)
    switch (D->F[i].type) {
      case 'I': if (i == 1) pF[0] = D->F[i];
      case 'P': if (i + nB - 1 < D->nF - 1) pF[i + nB - 1] = D->F[i];
                else pF[D->nF - 2] = D->F[i];
                break;
      case 'B': if (i >= 2) pF[i - 2] = D->F[i];
      default : break;
    }

  free(D->F);
  D->F = pF;

  return 1;
}

unsigned long MaxPackSize(dump_t *P, unsigned l)
{
  unsigned i;
  unsigned long max = P[0].size;

  for (i=1; i<l; i++) if (P[i].size > max) max = P[i].size;

  return 8 * max;
}
