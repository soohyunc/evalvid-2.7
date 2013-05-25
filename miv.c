
#if defined (_WIN32)
  #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
  #include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dir.h"

#define IV 10

static int cmpstr(const void *p1, const void *p2)
{
  return strcmp(*(char **)p1, *(char **)p2);
}

int main(int cn, char *cl[])
{
  FILE *f;
  char fn[1024], buf[1024], dir[512];
  char **F = 0;
  int i, j, nF = 0, nM = 0;
  double *M = 0, *pM, max;

  if (cn < 2) {
    puts("miv <dir>");
    puts("  dir\t\tdirectory with miv*.txt files");
    return 0;
  }

#if defined(_WIN32)
  if (cn < 2 || !SetCurrentDirectory(cl[1])) {
    fputs("Couldn't find directory.", stderr);
    return EXIT_FAILURE;
  }
  GetCurrentDirectory(sizeof dir, dir);
#elif defined(__linux__) || defined(__APPLE__)
  if (cn < 2 || chdir(cl[1]) == -1) {
    fputs("Couldn't find directory.\n", stderr);
    return EXIT_FAILURE;
  }
  getcwd(dir, sizeof dir);
#endif

  F = GetFiles(&nF, "miv*.txt");
  qsort(F, nF, sizeof *F, cmpstr);

  for (i=0; i<nF; i++) {
    strcat(strcat(strcpy(fn, dir), "/"), F[i]);
    if ((f = fopen(fn, "r")) == 0) {
      fprintf(stderr, "Couldn't open %s.\n", fn);
      continue;
    }
    nM = 0;
    while (!feof(f)) {
      if (!fgets(buf, sizeof buf, f)) break;
      if ((pM = realloc(M, ++nM * sizeof *M)) == 0) {
        fprintf(stderr, "realloc failed!");
        return EXIT_FAILURE;
      }
      M = pM;
      sscanf(buf, "%*d %lf", &M[nM - 1]);
    }

    fclose(f);

    max = nM ? M[0] : 999;
    for (j=1; j<nM; j++) if (M[j] > max) max = M[j];
    printf("%s\t%.01f\n", F[i], max);

    free(M); M = 0;
  }

  F = FreeStrAr(F, &nF);

  return 0;
}
