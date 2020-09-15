/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <stdio.h>
#include <string.h>
#include "pmu.h"
#include "utils.h"

#define FNAME "bench_results"

int
utils_compare(const void *a, const void *b)
{
	const cycle_t ai = *(const cycle_t *)a;
	const cycle_t bi = *(const cycle_t *)b;

	if (ai < bi) {
		return -1;
	} else if (ai > bi) {
		return 1;
	} else {
		return 0;
	}
}

void
utils_eliminate_zero(cycle_t *results)
{
	int i;

	for (i = 0; i < ITERATION; i++) {
		if (results[i] != 0) { break; }
	}
	for (int j = i - 1; j >= 0; j--) { results[j] = results[j + 1]; }
}

void
utils_clean_results(cycle_t *results)
{
	memset(results, 0, sizeof(cycle_t) * ITERATION);
}

void
utils_print_summary(const char *string, cycle_t *results)
{
	printf("%s:\n", string);
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);

#if __i386__ || __x86_64__
	printf("  results:\n    min: %llu\n    median: %llu\n    max: %llu\n", results[0], results[ITERATION / 2],
	       results[ITERATION - 1]);
#elif __arm__
	printf("  results:\n    min: %u\n    median: %u\n    max: %u\n", results[0], results[ITERATION / 2],
	       results[ITERATION - 1]);
#endif
}

void
utils_store_header(const char *string)
{
	FILE *f;

	f = fopen(FNAME, "a");
	if (f == NULL) { return; }

	fprintf(f, "%s\n", string);

	fclose(f);
}


void
utils_store_results(cycle_t *results)
{
	FILE *f;

	f = fopen(FNAME, "a");
	if (f == NULL) { return; }

#if __i386__ || __x86_64__
	for (int i = 0; i < ITERATION; i++) { fprintf(f, "Ticks (latency): %llu\n", results[i]); }
#elif __arm__
	for (int i = 0; i < ITERATION; i++) { fprintf(f, "Ticks (latency): %u\n", results[i]); }
#endif

	fclose(f);
}
