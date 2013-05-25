
#include <limits.h>
#include "bits.h"

static unsigned bit_ = CHAR_BIT, pos_;

void resetbits(unsigned pos)
{
  pos_ = pos;
  bit_ = CHAR_BIT;
}

unsigned nextbits(unsigned char *s, unsigned n)
{
  unsigned i = 0, v = 0;

  while (i < n) {
    if (!bit_) pos_++, bit_ = CHAR_BIT;
    if (bit_ == CHAR_BIT && (n - i) / CHAR_BIT) {
      while ((n - i) / CHAR_BIT) {
        v = v << CHAR_BIT | s[pos_++];
        i += CHAR_BIT;
      }
      continue;
    }
    v = v << 1 | GETBIT(s[pos_], --bit_);
    i++;
  }

  return v;
}

unsigned skipbits(unsigned char *s, unsigned b)
{
  unsigned count = 0;

  if (b > 1) return 0;

  for (;;) {
    if (!bit_) pos_++, bit_ = CHAR_BIT;
    if (b == GETBIT(s[pos_], --bit_)) count++; else break;
  }

  return count;
}

unsigned currentpos()
{
  return pos_;
}

unsigned currentbit()
{
  return bit_;
}

int mark_not_coded(unsigned char *p, unsigned n, unsigned nti)
{
  unsigned sc;

  resetbits(0);

  if ((sc = nextbits(p, 32)) == SC_GVOP) {
    while (currentpos() < n - 4) {
      if (nextbits(p, 32) == SC_VOP) goto VOP;
      resetbits(currentpos() - 3);
    }
    goto NO_VOP;
  }
  if (sc != SC_VOP) goto X;
VOP:
  nextbits(p, 2);               /* VOP type */
  skipbits(p, 1);               /* modulo_time_base */
  if (!nextbits(p, 1)) goto X;  /* marker bit */
  nextbits(p, nti);             /* vop_time_increment */
  if (!nextbits(p, 1)) goto X;  /* marker bit */
  CLRBIT(p[pos_], bit_ - 1);    /* set VOP not coded */
  if (nextbits(p, 1)) goto X;   /* VOP coded ? */
NO_VOP:
  return currentpos();

X: return 0;
}
