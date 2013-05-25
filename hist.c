
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stat.h"

int main(int cn, char **cl)
{
  FILE *f;
  void *tmp;
  double min, max, d, *D = 0;
  unsigned iv, line = 0, l = 0, num = 0;
  char buf[0xFF], *e;

  if (cn != 5) {
    puts("usage: hist <in> <min> <max> <iv>");
    puts(" <in>     input file (- for stdin)");
    puts(" <min>    lower interval bound");
    puts(" <max>    higher interval bound");
    puts(" <iv>     number of intervals");
    return 0;
  }

  if (!(min = strtod(cl[2], 0))) min = 0;
  if (!(max = strtod(cl[3], 0))) max = 1;
  if (!(iv = strtoul(cl[4], 0, 10))) iv = 100;

  if (!strcmp(cl[1], "-")) f = stdin;
  else
    if ((f = fopen(cl[1], "r")) == 0) {
      fprintf(stderr, "Could not open %s\n", cl[1]);
      return EXIT_FAILURE;
    }

  while (line++, fgets(buf, sizeof buf, f) && !feof(f) && !ferror(f)) {
    if (!(d = strtod(buf, &e)) && e == buf) {
      fprintf(stderr, "malformed input (%s, %u)\n", cl[1], line);
      continue;
    }

    if (l >= num) {
      num += 16384;
      if ((tmp = realloc(D, num * sizeof *D)) == 0) {
        fprintf(stderr, "realloc error\n");
        return 0;
      }
      D = tmp;
    }

    D[l++] = d;
  }

  Hist(D, l, min, max, iv);

  return 0;
}
