#ifndef BARRIER_H
#define BARRIER_H

#include <assert.h>

#define load(addr) (*(volatile __typeof__(*addr) *)(addr))

struct simple_barrier {
	unsigned long barrier;
	unsigned int  ncore;
};

static inline long
faa(unsigned long *target, long inc)
{
	__asm__ __volatile__("lock; xadd %1, %0" : "+m"(*target), "+q"(inc) : : "memory", "cc");
	return inc;
}

static void
simple_barrier(struct simple_barrier *b)
{
	unsigned long *barrier = &b->barrier;
	unsigned int   ncore   = b->ncore;

	assert(*barrier <= ncore);
	faa(barrier, 1);
	while (load(barrier) < ncore)
		;
}

static inline void
simple_barrier_init(struct simple_barrier *b, unsigned int ncores)
{
	*b = (struct simple_barrier){.barrier = 0, .ncore = ncores};
}

#endif /* BARRIER_H */