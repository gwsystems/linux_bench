/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "pmu.h"
#include "priority.h"
#include "utils.h"

pthread_t        t1, t2;
int              pp[2], pp2[2];
volatile cycle_t global_ts;
volatile int     count;
cycle_t *        results;

void
t1_work()
{
	cycle_t t;

	read(pp[0], &t, sizeof(cycle_t));
	write(pp2[1], &t, sizeof(cycle_t));

	while (count < ITERATION) {
		global_ts = get_cyclecount();
		sched_yield();

		t = get_cyclecount();

		if (t > global_ts) {
			results[count] = t - global_ts;
			count++;
		}
	}
}

void
t2_work()
{
	cycle_t t;

	write(pp[1], &t, sizeof(cycle_t));
	read(pp2[0], &t, sizeof(cycle_t));

	while (count < ITERATION) {
		global_ts = get_cyclecount();
		sched_yield();

		t = get_cyclecount();

		if (t > global_ts) {
			results[count] = t - global_ts;
			count++;
		}
	}
}

void *
t1_thread(void *p)
{
	pthread_prio(SCHED_FIFO, t1, 0);

	t1_work();

	return NULL;
}

void *
t2_thread(void *p)
{
	pthread_prio(SCHED_FIFO, t2, 0);

	t2_work();

	return NULL;
}

void
cs_bench_threads()
{
	utils_clean_results(results);

	count = 0;
	pthread_create(&t1, NULL, t1_thread, NULL);
	pthread_create(&t2, NULL, t2_thread, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	printf("context switch + thread:\n");

	utils_store_results(results, "cs_bench-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "cs_bench.csv");
}

void
cs_bench_processes()
{
	pid_t p;
	int   status;

	utils_clean_results(results);

	count = 0;

	p = fork();
	if (p == 0) {
		set_prio(SCHED_FIFO, 0);
		t2_work();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_FIFO, 0);
		t1_work();
	}

	waitpid(p, &status, 0);

	printf("context switch + processes:\n");

	utils_store_results(results, "cs_bench-p-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "cs_bench-p.csv");
}

int
main(int argc, char *argv[])
{
	set_affinity();
	set_prio(SCHED_FIFO, 2);

	pipe(pp);
	pipe(pp2);

	init_perfcounters(1, 0);

	results = malloc(sizeof(cycle_t) * ITERATION);

	cs_bench_threads();
	cs_bench_processes();

	close(pp[0]);
	close(pp[1]);
	close(pp2[0]);
	close(pp2[1]);

	free(results);

	return 0;
}
