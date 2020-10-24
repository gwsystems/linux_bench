/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pmu.h"
#include "priority.h"
#include "utils.h"

cycle_t *results;

void
getpid_bench()
{
	cycle_t t, tt;

	utils_clean_results(results);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();

		getpid();

		tt = get_cyclecount();

		if (tt > t) { results[i] = tt - t; }
	}

	utils_store_header("#####################BeginBench25-getpid");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("getpid", results);
	utils_store_header("#####################EndBench25-getpid");
}

void
getppid_bench()
{
	cycle_t t, tt;

	utils_clean_results(results);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();

		getppid();

		tt = get_cyclecount();

		if (tt > t) { results[i] = tt - t; }
	}

	utils_store_header("#####################BeginBench26-getppid");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("getppid", results);
	utils_store_header("#####################EndBench26-getppid");
}

void
close_bench()
{
	cycle_t t, tt;

	utils_clean_results(results);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();

		close(999);

		tt = get_cyclecount();

		if (tt > t) { results[i] = tt - t; }
	}

	utils_store_header("#####################BeginBench27-close");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("close", results);
	utils_store_header("#####################EndBench27-close");
}


int
main(int argc, char *argv[])
{
	set_affinity();
	set_prio(SCHED_RR, 0);

	results = malloc(sizeof(cycle_t) * ITERATION);

	init_perfcounters(1, 0);

	getpid_bench();
	getppid_bench();
	close_bench();

	free(results);

	return 0;
}
