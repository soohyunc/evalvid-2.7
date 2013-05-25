#ifndef TRACE_BITS
#define TRACE_BITS

#define GETBIT(x, n) ((unsigned) !!((x) & 1U << (n)))
#define SETBIT(x, n) ((x) |= 1U << (n))
#define TGLBIT(x, n) ((x) ^= 1U << (n))
#define CLRBIT(x, n) ((x) &= ~(1U << (n)))

#ifdef __cplusplus
extern "C" {
#endif

void resetbits(unsigned);
unsigned nextbits(unsigned char *, unsigned);
unsigned skipbits(unsigned char *, unsigned);
unsigned currentpos();
unsigned currentbit();

/* MPEG-4 */

#define SC_VOP  0x1b6
#define SC_GVOP 0x1b3

int mark_not_coded(unsigned char *p, unsigned n, unsigned nti);

#ifdef __cplusplus
}
#endif

#endif
