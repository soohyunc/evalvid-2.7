
#if defined (_WIN32)
  #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
  #include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dir.h"

#define FQ "qual.txt"

static int cmpstr(const void *p1, const void *p2)
{
  return strcmp(*(char **)p1, *(char **)p2);
}

int main(int cn, char *cl[])
{
  FILE *fi, *fo;
  char buf[1024], dir[512];
  char **F = 0, **Q = 0, *R = 0, **ppT, *pT;
  int i, j, k, l, iv, tmp, nF = 0, nQ = 0, nL = 0, nR = 0;
  int *L = 0, mos[5], *pL;
  double d;

  if (cn < 3) {
    puts("mos <dir> <ref> [iv]");
    puts("  dir\t\tdirectory with psnr*.txt files");
    puts("  ref\t\treference psnr file");
    puts("  iv\t\toptional nr. of frames in interval");
    return 0;
  }

  if (0 == (fi = fopen(cl[2], "r"))) {
    fprintf(stderr, "Coudn't open %s.\n", cl[2]);
    return EXIT_FAILURE;
  }

#if defined(_WIN32)
  if (!SetCurrentDirectory(cl[1])) {
    fputs("Couldn't find directory.\n", stderr);
    return EXIT_FAILURE;
  }
  GetCurrentDirectory(sizeof dir, dir);
#elif defined(__linux__) || defined(__APPLE__)
  if (chdir(cl[1]) == -1) {
    fputs("Couldn't find directory.\n", stderr);
    return EXIT_FAILURE;
  }
  getcwd(dir, sizeof dir);
#endif

  while (!feof(fi)) {
    if (!fgets(buf, sizeof buf, fi)) break;
    if ((d = strtod(buf, 0)) == 0) continue;
    if ((pT = realloc(R, ++nR * sizeof *R)) == 0) break;
    R = pT;
    R[nR-1] = d > 37 ? 5 : d > 31 ? 4 : d > 25 ? 3 : d > 20 ? 2 : 1;
  }

  fclose(fi);

  if (cn < 4) iv = 25;
  else {
    iv = strtol(cl[3], 0, 10);
    if (iv == 0) iv = 25;
  }

  if ((fo = fopen(FQ, "w")) == 0) {
    fputs("Couldn't open " FQ, stderr);
    return EXIT_FAILURE;
  }

  F = GetFiles(&nF, "psnr*.txt");
  qsort(F, nF, sizeof *F, cmpstr);

  for (j=0; j<nF; j++) {
    strcat(strcat(strcpy(buf, dir), "/"), F[j]);

    if ((fi = fopen(buf, "r")) == 0) {
      fprintf(stderr, "Could't open %s.\n", buf);
      continue;
    }

    if ((ppT = realloc(Q, ++nQ * sizeof *Q)) == 0) break;
    Q = ppT;
    Q[nQ - 1] = 0;

    if ((pL = realloc(L, nQ * sizeof *L)) == 0) break;
    L = pL;
    L[nQ - 1] = 0;

    nL = 0;
    while (!feof(fi)) {
      if (!fgets(buf, sizeof buf, fi)) break;
      if ((d = strtod(buf, 0)) == 0) continue;
      if ((pT = realloc(Q[nQ - 1], ++nL * sizeof *Q[nQ - 1])) == 0) break;
      Q[nQ - 1] = pT;
      Q[nQ - 1][nL - 1] = d > 37 ? 5 : d > 31 ? 4 : d > 25 ? 3 : d > 20 ? 2 : 1;
    }
    L[nQ - 1] = nL;

    fclose(fi);
  }

  for (k=0; k<nL; k++) {
    for (l=0; l<nQ; l++) fprintf(fo, "%2d", k < L[l] ? Q[l][k] : 0);
    fprintf(fo, "\n");
  }

  fclose(fo);

  for (k=0; k<nQ; k++) {
    printf("%s\t\t", F[k]);
    for (j=0; j<5; j++)    mos[j] = 0;
    for (l=0; l<L[k]; l++) mos[Q[k][l] - 1]++;
    for (d=0, j=0; j<5; j++) {
      d += (j + 1) * mos[j];
      printf("%8.2f", 100. * mos[j] / L[k]);
    }
    printf("   %8.2f\n", d / L[k]);
  }

  for (k=0; k<nQ; k++) {
    char s[0xff] = "miv";
    strcat(s, 1 + strchr(F[k], 'r'));
    if ((fo = fopen(s, "w")) == 0) {
      fprintf(stderr, "Couldn't open %s.\n", s);
      return EXIT_FAILURE;
    }

    tmp = L[k] < iv ? L[k] : iv;
    for (l=0; l<=L[k]-tmp; l++) {
      for (i=0, j=l; j<l+tmp; j++) i += Q[k][j] < R[j] && Q[k][j] < 4;
      fprintf(fo, "%d\t%6.2f\n", l + tmp, 100. * i / tmp);
    }

    fclose(fo);
  }

  F = FreeStrAr(F, &nF);
  Q = FreeStrAr(Q, &nQ);

  return 0;
}
