
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "writemp4.h"
#include "read.h"
#include "stat.h"

int main(int cn, char **cl)
{
  unsigned PoB = 0, num;
  char id[1024] = {0}, *p;
  static data_t D;
  static loss_t loss;
  dump_t *dump;
  MODE mode = INVALID;
  int l, n = 0, o = 2;

  if (cn < 8) {
U:  puts("usage: et -[p|f|F] -[0|x] [-c] <sd> <rd> <st> <in> <out> [PoB]");
    puts("-[p|f|F] packet, frame or complete frame mode (alternative)");
    puts("-[0|x] fill lost section with 0 or truncate (alternative)");
    puts("  [-c] use cumulative jitter in case of asynchronous clocks (optional)");
    puts("  <sd> tcpdump sender");
    puts("  <rd> tcpdump receiver");
    puts("  <st> trace-file sender");
    puts("  <in> transmitted video (original mp4)");
    puts("  <out> base name of output-file");
    puts(" [PoB] optional Play-out buffer size [ms]");
    return 0;
  }

  while (++n < cn - 1) {
    p = cl[n];
    if (*p++ != '-') break;
    switch(*p) {
      case 'p': if (mode & FRAME) goto U; mode |= PACKET; break;
      case 'f': if (mode & PACKET) goto U; mode |= FRAME; break;
      case 'F': if (mode & PACKET || mode & FRAME) goto U; mode |= COMPLETE; break;
      case '0': if (mode & TRUNC) goto U; mode |= FILL; break;
      case 'x': if (mode & FILL) goto U; mode |= TRUNC; break;
      case 'c': o++; mode |= ASYNC; break;
      default : goto U;
    }
  }

  if (cn == o + 7) {
    PoB = strtoul(cl[o + 6], 0, 10);
    if (PoB < 1 || PoB > 1000000) PoB = 1000000;
  }
  l = (int) strlen(cl[o + 5]);
  while (l-- && cl[o + 5][l] != '\\' && cl[o + 5][l] != '/');
  if (p = strrchr(strncpy(id, cl[o + 5] + l + 1, sizeof id - 10), '.')) *p = 0;

  if (!ReadDump(cl + o, &D, mode, 0)) return EXIT_FAILURE;
  if (D.F[0].type == 'A') mode |= AUDIO;

  dump = mode & FRAME || mode & COMPLETE ? D.F : D.P;
  num = mode & FRAME || mode & COMPLETE ? D.nF : D.nP;

  CalcJitter(dump, num);
  if (PoB) PoBLoss(dump, num, PoB, mode);
  CalcLoss(dump, num, &loss, id);
  OutJitter(dump, num, id);
  CalcSRate(dump, num, 1.0, id);
  CalcRRate(dump, num, 1.0, id);

  if (!WriteMP4(cl[o + 4], id, &D, mode)) return EXIT_FAILURE;

  free(D.P);
  free(D.F);

  return 0;
}
