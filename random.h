#ifndef JKL_RAND
#define JKL_RAND

void init_rand(void);
void init_genrand(unsigned long);
void init_by_array(unsigned long *, int);
unsigned long genrand_int32(void);

/* generates a random number on [0,0x7fffffff]-interval */
long genrand_int31(void);

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void);

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void);

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void);

/* generates a random number on [0,1) with 53-bit resolution */
double genrand_res53(void);

int Gauss(int);

/* generates a random numbers on [from, to] */
unsigned long rand_range(int, int);

void init_GE(int, int, int);

int GilbertElliot(int);

#endif
