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

pthread_t t1, t2;
int       pp[2], pp2[2], pp3[2], pp4[2];
cycle_t * results, *results2, *results3;

void
reader_work()
{
	cycle_t ts;

	CHECK(read(pp3[0], &ts, sizeof(cycle_t)));
	CHECK(write(pp4[1], &ts, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(pp[0], &ts, sizeof(cycle_t)));
		ts = get_cyclecount();
		CHECK(write(pp2[1], &ts, sizeof(cycle_t)));
	}
}

void
writer_work()
{
	cycle_t ts, ts2, ts3;

	CHECK(write(pp3[1], &ts, sizeof(cycle_t)));
	CHECK(read(pp4[0], &ts, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		ts = get_cyclecount();
		CHECK(write(pp[1], &ts, sizeof(cycle_t)));
		CHECK(read(pp2[0], &ts2, sizeof(cycle_t)));
		ts3 = get_cyclecount();

		if (ts2 > ts && ts3 > ts2) {
			results[i]  = ts2 - ts;
			results2[i] = ts3 - ts2;
			results3[i] = ts3 - ts;
		}
	}
}


void *
reader_thread(void *p)
{
	pthread_prio(SCHED_RR, t1, *(int *)p);

	reader_work();

	return NULL;
}

void *
writer_thread(void *p)
{
	pthread_prio(SCHED_RR, t2, *(int *)p);

	writer_work();

	return NULL;
}

void
oneway_bench_processes()
{
	pid_t p;
	int   status;

	printf("pipe + between processes:\n");
	utils_clean_results(results);
	utils_clean_results(results2);
	utils_clean_results(results3);

	p = fork();
	if (p == 0) {
		set_prio(SCHED_RR, 0);
		reader_work();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_RR, 1);
		writer_work();
	}

	waitpid(p, &status, 0);
	utils_store_results(results, "pipe_bench-p_1w_rh-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-p_1w_rh.csv");

	utils_store_results(results2, "pipe_bench-p_1w_wh-unsorted.csv");
	qsort(results2, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results2);
	utils_print_results(results2);
	utils_store_results(results2, "pipe_bench-p_1w_wh.csv");

	utils_store_results(results3, "pipe_bench-p_rt-unsorted.csv");
	qsort(results3, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results3);
	utils_print_results(results3);
	utils_store_results(results3, "pipe_bench-p_rt.csv");
}

void
oneway_bench_threads()
{
	int n1, n2;

	printf("pipe + between threads:\n");
	utils_clean_results(results);
	utils_clean_results(results2);
	utils_clean_results(results3);

	n1 = 0;
	n2 = 1;
	pthread_create(&t1, NULL, reader_thread, &n1);
	pthread_create(&t2, NULL, writer_thread, &n2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_results(results, "pipe_bench-t_1w_rh-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-t_1w_rh.csv");

	utils_store_results(results2, "pipe_bench-t_1w_wh-unsorted.csv");
	qsort(results2, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results2);
	utils_print_results(results2);
	utils_store_results(results2, "pipe_bench-t_1w_wh.csv");

	utils_store_results(results3, "pipe_bench-t_rt-unsorted.csv");
	qsort(results3, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results3);
	utils_print_results(results3);
	utils_store_results(results3, "pipe_bench-t_rt.csv");
}

int
main(int argc, char *argv[])
{
	set_affinity();
	set_prio(SCHED_RR, 2);

	pipe(pp);
	pipe(pp2);
	pipe(pp3);
	pipe(pp4);

	results  = malloc(sizeof(cycle_t) * ITERATION);
	results2 = malloc(sizeof(cycle_t) * ITERATION);
	results3 = malloc(sizeof(cycle_t) * ITERATION);

	init_perfcounters(1, 0);

	oneway_bench_threads();
	oneway_bench_processes();

	close(pp[0]);
	close(pp[1]);
	close(pp2[0]);
	close(pp2[1]);
	close(pp3[0]);
	close(pp3[1]);
	close(pp4[0]);
	close(pp4[1]);

	free(results);
	free(results2);
	free(results3);

	return 0;
}
