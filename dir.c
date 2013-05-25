
#if defined(_WIN32)
  #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
  #include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int match(char *s, char *pattern)
{
  char a[1024], *p;
  int r = 1;

  strcpy(a, pattern);

  p = strtok(a, "*");
  while (p && r) {
    if (!strstr(s, p)) r = 0;
    p = strtok(0, "*");
  }

  return r;
}

#include "dir.h"

char **FreeStrAr(char **s, int *l)
{
  int i = 0;

  for (; i<*l; i++) {
    free(s[i]);
    s[i] = 0;
  }
  free(s); s = 0; *l = 0;

  return s;
}

char **GetFiles(int *nF, char *s)
{
  char **F = 0, **tmp;

#if defined(_WIN32)
  HANDLE h = 0;
  WIN32_FIND_DATA d = {0};

  if (INVALID_HANDLE_VALUE != (h = FindFirstFile(s, &d))) {
    if (d.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
      F = realloc(F, ++*nF * sizeof *F);
      F[*nF - 1] = malloc((strlen(d.cFileName) + 1));
      strcpy(F[*nF - 1], d.cFileName);
    }
    while (FindNextFile(h, &d))
      if (d.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
        if ((tmp = realloc(F, ++*nF * sizeof *F)) == 0) break;
        F = tmp;
        if ((F[*nF - 1] = malloc(strlen(d.cFileName) + 1)) == 0) break;
        strcpy(F[*nF - 1], d.cFileName);
      }
  }
  FindClose(h);
#elif defined(__linux__) || defined(__APPLE__)
  struct dirent *de;
  DIR *dir = opendir(".");

  if (dir) {
    while (de = readdir(dir)) {
      if (match(de->d_name, s)) {
        if (!(tmp = realloc(F, ++*nF * sizeof *F))) {
          fprintf(stderr, "realloc failed.");
          return 0;
        }
        F = tmp;
        F[*nF - 1] = malloc((strlen(de->d_name) + 1));
        strcpy(F[*nF - 1], de->d_name);
      }
    }
  }
  closedir(dir);
#endif

  return F;
}
