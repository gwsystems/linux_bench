/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pmu.h"
#include "priority.h"
#include "utils.h"

pthread_t           t1, t2;
pthread_mutex_t     lock;
pthread_mutexattr_t lock_attr;
int                 pp[2];
volatile int        flag;
cycle_t *           results;


void *
no_contend_thread(void *p)
{
	cycle_t t, tt;

	pthread_prio(SCHED_RR, t1, *(int *)p);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();

		pthread_mutex_lock(&lock);
		pthread_mutex_unlock(&lock);

		tt = get_cyclecount();
		if (tt < t) { continue; }

		results[i] = tt - t;
	}

	return NULL;
}


void *
t2_contend_thread(void *p)
{
	cycle_t t, tt;

	pthread_prio(SCHED_RR, t2, *(int *)p);

	for (int i = 0; i < ITERATION; i++) {
		CHECK(write(pp[1], &t, sizeof(cycle_t)));

		usleep(300);

		flag = 1;
		t    = get_cyclecount();

		pthread_mutex_lock(&lock);
		pthread_mutex_unlock(&lock);

		tt = get_cyclecount();
		if (tt > t) { results[i] = tt - t; }
	}

	return NULL;
}

void *
t1_contend_thread(void *p)
{
	cycle_t t;
	pthread_prio(SCHED_RR, t1, *(int *)p);

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(pp[0], &t, sizeof(cycle_t)));

		flag = 0;

		pthread_mutex_lock(&lock);

		while (flag != 1) {}

		pthread_mutex_unlock(&lock);
	}

	return NULL;
}

void
mutex_bench()
{
	int n1, n2;

	printf("mutex + no contender:\n");
	utils_clean_results(results);

	n1 = 0;
	pthread_mutex_init(&lock, NULL);
	pthread_create(&t1, NULL, no_contend_thread, &n1);
	pthread_join(t1, NULL);
	pthread_mutex_destroy(&lock);

	utils_store_results(results, "mutex_bench-no_con-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "mutex_bench-no_con.csv");

	printf("mutex + contender:\n");
	flag = 0;
	utils_clean_results(results);

	n1 = 1;
	n2 = 0;
	pthread_mutex_init(&lock, NULL);
	pthread_create(&t2, NULL, t2_contend_thread, &n2);
	pthread_create(&t1, NULL, t1_contend_thread, &n1);
	pthread_join(t2, NULL);
	pthread_join(t1, NULL);
	pthread_mutex_destroy(&lock);

	utils_store_results(results, "mutex_bench-con-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "mutex_bench-con.csv");

	printf("mutex + priority inheritance + no contender:\n");
	utils_clean_results(results);

	n1 = 0;
	pthread_mutexattr_init(&lock_attr);
	pthread_mutexattr_setprotocol(&lock_attr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&lock, &lock_attr);
	pthread_create(&t1, NULL, no_contend_thread, &n1);
	pthread_join(t1, NULL);
	pthread_mutex_destroy(&lock);

	utils_store_results(results, "mutex_bench-pi_no_con-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "mutex_bench-pi_no_con.csv");

	printf("mutex latency + priority inheritance + contender:\n");
	flag = 0;
	utils_clean_results(results);

	n1 = 1;
	n2 = 0;
	pthread_mutexattr_init(&lock_attr);
	pthread_mutexattr_setprotocol(&lock_attr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&lock, &lock_attr);
	pthread_create(&t2, NULL, t2_contend_thread, &n2);
	pthread_create(&t1, NULL, t1_contend_thread, &n1);
	pthread_join(t2, NULL);
	pthread_join(t1, NULL);
	pthread_mutex_destroy(&lock);

	utils_store_results(results, "mutex_bench-pi_con-unsorted.csv");
	qsort(results, ITERATION, sizeof(cycle_t), utils_compare);
	utils_eliminate_zero(results);
	utils_print_results(results);
	utils_store_results(results, "mutex_bench-pi_con.csv");
}


int
main(int argc, char *argv[])
{
	set_affinity();
	set_prio(SCHED_RR, 2);
	pipe(pp);

	results = malloc(sizeof(cycle_t) * ITERATION);

	init_perfcounters(1, 0);

	mutex_bench();

	close(pp[0]);
	close(pp[1]);
	free(results);

	return 0;
}
