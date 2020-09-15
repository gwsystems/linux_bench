/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pmu.h"
#include "priority.h"
#include "utils.h"

#define SEMAPHORE_NAME "bench"

pthread_t    t1, t2;
sem_t        lock;
int          pp[2];
volatile int flag;
cycle_t *    results;


void *
no_contend_thread(void *p)
{
	cycle_t t, tt;

	pthread_prio(SCHED_RR, t1, *(int *)p);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();

		sem_wait(&lock);
		sem_post(&lock);

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

		sem_wait(&lock);
		sem_post(&lock);

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

		sem_wait(&lock);

		while (flag != 1) {}

		sem_post(&lock);
	}

	return NULL;
}

void
mutex_bench()
{
	int n1, n2;

	utils_clean_results(results);

	n1 = 0;
	sem_init(&lock, 0, 1);
	pthread_create(&t1, NULL, no_contend_thread, &n1);
	pthread_join(t1, NULL);
	sem_destroy(&lock);

	utils_store_header("#####################BeginBench11-semaphore-no-contender");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("semaphore + no contender", results);
	utils_store_header("#####################EndBench11-semaphore-no-contender");

	flag = 0;
	utils_clean_results(results);

	n1 = 1;
	n2 = 0;
	sem_init(&lock, 0, 1);
	pthread_create(&t2, NULL, t2_contend_thread, &n2);
	pthread_create(&t1, NULL, t1_contend_thread, &n1);
	pthread_join(t2, NULL);
	pthread_join(t1, NULL);
	sem_destroy(&lock);

	utils_store_header("#####################BeginBench12-semaphore-contender");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("semaphore + contender", results);
	utils_store_header("#####################EndBench12-semaphore-contender");
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
