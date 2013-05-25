
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc.h"

int ToLower(int c)
{
  static const char lower[] = "abcdefghijklmnopqrstuvwxyzäöü";
  static const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ";

  char *p = strchr(upper, c);
  return p ? lower[p - upper] : c;
}

int casecmp(const char *s, const char *t)
{
  while (*s && *t && ToLower(*s) == ToLower(*t)) {
    s++;
    t++;
  }
  return *s - *t;
}

char *dupstr(const char *s)
{
  size_t l = strlen(s) + 1;
  char *p = malloc(l);

  if (p) memcpy(p, s, l);

  return p;
}

char *skipc(char *p, const char *s)
{
  unsigned i, f = 1;
  size_t l = strlen(s);

  while (f)
    for (f = 0, i = 0; i < l; i++)
      if (*p == s[i]) {
        p++;
        f = 1;
        break;
      }

  return p;
}

char *skips(char *p, const char *s, int n)
{
  int i;
  size_t l = strlen(s);

  for (i = 0; i < n; i++)
    if ((p = strstr(p, s)) != 0) p += l;
    else break;

  return p;
}

char *getstr(char *buf, const char *search, char **ret)
{
  char *p, *q;

  if (p = skips(buf, search, 1)) {
    p = skipc(p, " \t=\"");
    if (q = strrchr(p, '\"')) *q = 0;
    return *ret = dupstr(p);
  }

  return 0;
}

unsigned getuint(char *buf, const char *search, unsigned *ret)
{
  char *p;

  if (p = skips(buf, search, 1)) {
    p = skipc(p, " \t=");
    return *ret = strtoul(p, 0, 10);
  }

  return 0;
}

double getdbl(char *buf, const char *search, double *ret)
{
  char *p;

  if (p = skips(buf, search, 1)) {
    p = skipc(p, " \t=");
    return *ret = strtod(p, 0);
  }

  return 0;
}

unsigned neededbits(unsigned long n)
{
  unsigned l = 0;

  while (l++, n >>= 1)
    ;

  return l;
}

int copyfile(char *s, char *d)
{
  FILE *fi, *fo;
  char buf[BUFSIZ];
  size_t r;

  if ((fi = fopen(s, "rb")) == 0) {
    fprintf(stderr, "error opening %s\n", s);
    return 0;
  }
  if ((fo = fopen(d, "wb")) == 0) {
    fprintf(stderr, "error opening %s\n", d);
    return 0;
  }
  for (;;) {
    if (0 == (r = fread(buf, 1, sizeof buf, fi))) break;
    if (0 == fwrite(buf, 1, r, fo)) break;
  }
  fclose(fo);
  fclose(fi);

  return !ferror(fi) && !ferror(fo);
}
