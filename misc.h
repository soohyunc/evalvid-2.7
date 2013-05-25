#ifndef TRACE_MISC
#define TRACE_MISC

#ifdef __cplusplus
extern "C" {
#endif

int casecmp(const char *, const char *);
char *dupstr(const char *);
char *skips(char *, const char *, int);
char *skipc(char *, const char *);
char *getstr(char *, const char *, char **);
unsigned getuint(char *, const char *, unsigned *);
double getdbl(char *, const char *, double *);
unsigned neededbits(unsigned long);
int copyfile(char *, char *);

#ifdef __cplusplus
}
#endif

#endif
