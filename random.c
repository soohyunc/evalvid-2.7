
#include <time.h>

#include "random.h"

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL
#define UPPER_MASK 0x80000000UL
#define LOWER_MASK 0x7fffffffUL

static unsigned long mt[N];
static int mti = N + 1;
static double BERg, BERb, Pgb, Pbg;

void init_genrand(unsigned long s)
{
  mt[0]= s & 0xffffffffUL;
  for (mti=1; mti<N; mti++) {
    mt[mti] = (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
    mt[mti] &= 0xffffffffUL;
  }
}

void init_by_array(unsigned long init_key[], int key_length)
{
  int i=1, j=0, k;

  init_genrand(19650218UL);
  for (k = N > key_length ? N : key_length; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL)) + init_key[j] + j;
    mt[i] &= 0xffffffffUL;
    i++; j++;
    if (i >= N) { mt[0] = mt[N-1]; i = 1; }
    if (j >= key_length) j = 0;
  }
  for (k=N-1; k; k--) {
    mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL)) - i;
    mt[i] &= 0xffffffffUL;
    i++;
    if (i >= N) { mt[0] = mt[N-1]; i = 1; }
  }
  mt[0] = 0x80000000UL;
}

void init_rand(void)
{
  static unsigned long init[] = {0xAFFE, 0xDEAD, 0x42, 42, 0};
  static int l = sizeof init / sizeof *init;

  init[l - 1] = (unsigned long) time(0);
  init_by_array(init, l);
}

unsigned long genrand_int32(void)
{
  unsigned long y;
  static unsigned long mag01[2] = {0, MATRIX_A};

  if (mti >= N) {
    int kk;

    if (mti == N + 1) init_genrand(5489);

    for (kk=0; kk<N-M; kk++) {
      y = (mt[kk] & UPPER_MASK) | (mt[kk+1] & LOWER_MASK);
      mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 1UL];
    }
    for (; kk<N-1; kk++) {
      y = (mt[kk] & UPPER_MASK) | (mt[kk+1] & LOWER_MASK);
      mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 1UL];
    }
    y = (mt[N-1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 1UL];

    mti = 0;
  }

  y = mt[mti++];

  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  return y;
}

long genrand_int31(void)
{
  return genrand_int32() >> 1;
}

double genrand_real1(void)
{
  return genrand_int32() / 4294967295.;
}

double genrand_real2(void)
{
  return genrand_int32() / 4294967296.;
}

double genrand_real3(void)
{
  return (genrand_int32() + .5) / 4294967296.;
}

double genrand_res53(void)
{
  unsigned long a = genrand_int32() >> 5, b = genrand_int32() >> 6;

  return (a * 67108864. + b) / 9007199254740992.;
}

int Gauss(int p)
{
  return p / 2 == 1 + (int) (p * (double)genrand_int32() / 4294967296.);
}

unsigned long rand_range(int from, int to)
{
  return from + (int) ((to - from + 1) * (double)genrand_int32() / 4294967296.);
}

void init_GE(int BER, int stateratio, int bgratio)
{
  Pgb  = .001,
  Pbg  = stateratio * Pgb,
  BERg = (1. / BER) * (stateratio + 1) / (stateratio + bgratio);
  BERb = (1. / BER) * (stateratio + 1) * bgratio / (stateratio + bgratio);
}

int GilbertElliot(int ignore)
{
  static int state = 1;

  int bit = genrand_res53() < (state ? BERg : BERb);
  state   = genrand_res53() < (state ? Pgb : Pbg);

  (void)ignore;

  return bit;
}
