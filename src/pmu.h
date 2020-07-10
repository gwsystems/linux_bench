/* SPDX-License-Identifier: BSD-2-CLAUSE */

#ifndef PMU_H
#define PMU_H

#if __i386__ || __x86_64__
#include <x86intrin.h>
typedef unsigned long long int cycle_t;
#elif __arm__
typedef unsigned int cycle_t;
#endif


static __inline__ cycle_t
get_cyclecount(void)
{
	cycle_t value;
#if __i386__ || __x86_64__
	value = __rdtsc();
#elif __arm__
	asm __volatile__("MRC p15, 0, %0, c9, c13, 0\t\n" : "=r"(value));
#endif
	return value;
}

static __inline__ void
init_perfcounters(int do_reset, int enable_divider)
{
#if __arm__
	int value = 1;

	if (do_reset) {
		value |= 2;
		value |= 4;
	}

	if (enable_divider) value |= 8;

	value |= 16;

	asm volatile("MCR p15, 0, %0, c9, c12, 0\t\n" ::"r"(value));
	asm volatile("MCR p15, 0, %0, c9, c12, 1\t\n" ::"r"(0x8000000f));
	asm volatile("MCR p15, 0, %0, c9, c12, 3\t\n" ::"r"(0x8000000f));
#endif
}

#endif
