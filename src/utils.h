/* SPDX-License-Identifier: BSD-2-CLAUSE */

#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include "pmu.h"

#define ITERATION 10 * 1000 

#define CHECK(x)                                                                                           \
	do {                                                                                               \
		int retval = (x);                                                                          \
		if (retval < 0) {                                                                          \
			printf("check on line: %d failed. retval: %d errno: %d", __LINE__, retval, errno); \
			exit(EXIT_FAILURE);                                                                \
		}                                                                                          \
	} while (0)

int  utils_compare(const void *a, const void *b);
void utils_clean_results(cycle_t *results);
void utils_eliminate_zero(cycle_t *results);
void utils_print_summary(const char *string, cycle_t *results);
void utils_store_header(const char *string);
void utils_store_results(cycle_t *results);

#endif
