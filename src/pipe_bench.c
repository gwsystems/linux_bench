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
cycle_t * results;

void
reader_work_oneway()
{
	cycle_t t, tt;

	for (int i = 0; i < ITERATION; i++) {
		CHECK(write(pp4[1], &t, sizeof(cycle_t)));
		CHECK(read(pp3[0], &t, sizeof(cycle_t)));

		CHECK(read(pp[0], &t, sizeof(cycle_t)));
		tt = get_cyclecount();
		if (tt < t) { continue; }

		results[i] = tt - t;
	}
}

void
writer_work_oneway()
{
	cycle_t t;

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(pp4[0], &t, sizeof(cycle_t)));
		CHECK(write(pp3[1], &t, sizeof(cycle_t)));

		t = get_cyclecount();
		CHECK(write(pp[1], &t, sizeof(cycle_t)));
	}
}

void
reader_work_roundtrip()
{
	cycle_t t;

	CHECK(read(pp3[0], &t, sizeof(t)));
	CHECK(write(pp4[1], &t, sizeof(t)));

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(pp[0], &t, sizeof(cycle_t)));
		CHECK(write(pp2[1], &t, sizeof(cycle_t)));
	}
}

void
writer_work_roundtrip()
{
	cycle_t t, tt;

	CHECK(write(pp3[1], &t, sizeof(cycle_t)));
	CHECK(read(pp4[0], &t, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();
		CHECK(write(pp[1], &t, sizeof(cycle_t)));
		CHECK(read(pp2[0], &t, sizeof(cycle_t)));

		tt = get_cyclecount();
		if (tt < t) { continue; }

		results[i] = tt - t;
	}
}


void *
reader_thread(void *p)
{
	pthread_prio(SCHED_RR, t1, *(int *)p);

	reader_work_oneway();

	return NULL;
}

void *
writer_thread(void *p)
{
	pthread_prio(SCHED_RR, t2, *(int *)p);

	writer_work_oneway();

	return NULL;
}

void *
reader2_thread(void *p)
{
	pthread_prio(SCHED_RR, t1, *(int *)p);

	reader_work_roundtrip();

	return NULL;
}

void *
writer2_thread(void *p)
{
	pthread_prio(SCHED_RR, t2, *(int *)p);

	writer_work_roundtrip();

	return NULL;
}

void
oneway_bench_processes()
{
	pid_t p;
	int   status;

	printf("pipe + between processes + 1 way + reader w/ higher prio:\n");
	utils_clean_results(results);

	p = fork();
	if (p == 0) {
		set_prio(SCHED_RR, 1);
		writer_work_oneway();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_RR, 0);
		reader_work_oneway();
	}

	waitpid(p, &status, 0);
	utils_store_results(results, "pipe_bench-p_1w_rh-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-p_1w_rh.csv");

	printf("pipe + between processes + 1 way + writer w/ higher prio:\n");
	utils_clean_results(results);

	p = fork();
	if (p == 0) {
		set_prio(SCHED_RR, 0);
		writer_work_oneway();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_RR, 1);
		reader_work_oneway();
	}

	waitpid(p, &status, 0);
	utils_store_results(results, "pipe_bench-p_1w_wh-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-p_1w_wh.csv");

	printf("pipe + between processes + round trip:\n");
	utils_clean_results(results);

	p = fork();
	if (p == 0) {
		set_prio(SCHED_RR, 0);
		reader_work_roundtrip();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_RR, 1);
		writer_work_roundtrip();
	}

	waitpid(p, &status, 0);
	utils_store_results(results, "pipe_bench-p_rt-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-p_rt.csv");
}

void
oneway_bench_threads()
{
	int n1, n2;

	printf("pipe + between threads + 1 way + reader w/ higher prio:\n");
	utils_clean_results(results);

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

	printf("pipe + between threads + 1 way + writer w/ higher prio:\n");
	utils_clean_results(results);

	n1 = 1;
	n2 = 0;
	pthread_create(&t1, NULL, reader_thread, &n1);
	pthread_create(&t2, NULL, writer_thread, &n2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_results(results, "pipe_bench-t_1w_wh-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-t_1w_wh.csv");

	printf("pipe + between threads + round trip:\n");
	utils_clean_results(results);

	n1 = 0;
	n2 = 1;
	pthread_create(&t1, NULL, reader2_thread, &n1);
	pthread_create(&t2, NULL, writer2_thread, &n2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_results(results, "pipe_bench-t_rt-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "pipe_bench-t_rt.csv");
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

	results = malloc(sizeof(cycle_t) * ITERATION);

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

	return 0;
}
