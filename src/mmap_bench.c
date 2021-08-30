/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "barrier.h"
#include "pmu.h"
#include "priority.h"
#include "utils.h"

#undef ITERATION
#define ITERATION 100 * 1000

#define PAGE_SIZE 4096

struct simple_barrier b;
cycle_t *             results, *results2;

struct arg {
	void *p;
	int   core;
	int   n_cores;
};

void *
mmap_bench(void *args)
{
	char    title[32];
	cycle_t t, tt, ttt;
	int     core, n_cores;
	void *  p;

	core    = ((struct arg *)args)->core;
	n_cores = ((struct arg *)args)->n_cores;
	p       = ((struct arg *)args)->p;

	set_affinity2(core);

	utils_clean_results(results);
	utils_clean_results(results2);

	simple_barrier(&b);

	for (int i = 0; i < ITERATION; i++) {
		t  = get_cyclecount();
		p  = mmap(p, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                         -1, 0);
		tt = get_cyclecount();
		munmap(p, PAGE_SIZE);
		ttt = get_cyclecount();

		if (core == 0) {
			if (tt > t) { results[i] = tt - t; }

			if (ttt > tt) { results2[i] = ttt - tt; }
		}
	}

	if (core == 0) {
		sprintf(title, "%d cores: mmap w/ munmap: mmap", n_cores);
		utils_print_summary(title, results);

		sprintf(title, "%d cores: mmap w/ munmap: munmap", n_cores);
		utils_print_summary(title, results2);
	}

	return NULL;
}

void *
mmap_only_bench(void *args)
{
	char    title[32];
	cycle_t t, tt;
	int     core, n_cores;
	void *  p;

	core    = ((struct arg *)args)->core;
	n_cores = ((struct arg *)args)->n_cores;
	p       = ((struct arg *)args)->p;

	set_affinity2(core);

	utils_clean_results(results);

	simple_barrier(&b);

	for (int i = 0; i < ITERATION; i++) {
		t  = get_cyclecount();
		p  = mmap(p, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                         -1, 0);
		tt = get_cyclecount();
		p  = (char *)p + PAGE_SIZE;

		if (core == 0) {
			if (tt > t) { results[i] = tt - t; }
		}
	}

	if (core == 0) {
		sprintf(title, "%d cores: mmap", n_cores);
		utils_print_summary(title, results);
	}

	p = (char *)p - (PAGE_SIZE * ITERATION);
	munmap(p, PAGE_SIZE * ITERATION);

	return NULL;
}

void *
munmap_only_bench(void *args)
{
	char    title[32];
	cycle_t t, tt;
	int     core, n_cores;
	void *  p;

	core    = ((struct arg *)args)->core;
	n_cores = ((struct arg *)args)->n_cores;
	p       = ((struct arg *)args)->p;

	set_affinity2(core);

	utils_clean_results(results);

	p = mmap(p, PAGE_SIZE * ITERATION, PROT_EXEC | PROT_READ | PROT_WRITE,
	         MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

	simple_barrier(&b);

	for (int i = 0; i < ITERATION; i++) {
		t = get_cyclecount();
		munmap(p, PAGE_SIZE);
		tt = get_cyclecount();
		p  = (char *)p + PAGE_SIZE;

		if (core == 0) {
			if (tt > t) { results[i] = tt - t; }
		}
	}

	if (core == 0) {
		sprintf(title, "%d cores: munmap", n_cores);
		utils_print_summary(title, results);
	}

	return NULL;
}

int
main(int argc, char *argv[])
{
	int         n_cores = sysconf(_SC_NPROCESSORS_ONLN) / 2; // Assuming always run with HT enabled
	pthread_t   tid[n_cores];
	struct arg *args[n_cores];
	void *      p_pool[n_cores];

	set_affinity();

	results  = malloc(sizeof(cycle_t) * ITERATION);
	results2 = malloc(sizeof(cycle_t) * ITERATION);

	init_perfcounters(1, 0);

	for (int i = 1; i <= n_cores; i++) {
		simple_barrier_init(&b, i);

		for (int j = 0; j < i; j++) {
			args[j]   = malloc(sizeof(struct arg));
			p_pool[j] = mmap(NULL, PAGE_SIZE * ITERATION, PROT_EXEC | PROT_READ | PROT_WRITE,
			                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
		}

		for (int j = 0; j < i; j++) { munmap(p_pool[j], PAGE_SIZE * ITERATION); }

		// printf("Creating threads\n");
		for (int j = 0; j < i; j++) {
			args[j]->core    = j;
			args[j]->n_cores = i;
			args[j]->p       = p_pool[j];
			pthread_create(&tid[j], NULL, mmap_bench, args[j]);
		}

		for (int j = 0; j < i; j++) {
			pthread_join(tid[j], NULL);
			free(args[j]);
		}
	}

	for (int i = 1; i <= n_cores; i++) {
		simple_barrier_init(&b, i);

		for (int j = 0; j < i; j++) {
			args[j]   = malloc(sizeof(struct arg));
			p_pool[j] = mmap(NULL, PAGE_SIZE * ITERATION, PROT_EXEC | PROT_READ | PROT_WRITE,
			                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
		}

		for (int j = 0; j < i; j++) { munmap(p_pool[j], PAGE_SIZE * ITERATION); }

		// printf("Creating threads\n");
		for (int j = 0; j < i; j++) {
			args[j]->core    = j;
			args[j]->n_cores = i;
			args[j]->p       = p_pool[j];
			pthread_create(&tid[j], NULL, mmap_only_bench, args[j]);
		}

		for (int j = 0; j < i; j++) {
			pthread_join(tid[j], NULL);
			free(args[j]);
		}
	}

	for (int i = 1; i <= n_cores; i++) {
		simple_barrier_init(&b, i);

		for (int j = 0; j < i; j++) {
			args[j]   = malloc(sizeof(struct arg));
			p_pool[j] = mmap(NULL, PAGE_SIZE * ITERATION, PROT_EXEC | PROT_READ | PROT_WRITE,
			                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
		}

		for (int j = 0; j < i; j++) { munmap(p_pool[j], PAGE_SIZE * ITERATION); }

		// printf("Creating threads\n");
		for (int j = 0; j < i; j++) {
			args[j]->core    = j;
			args[j]->n_cores = i;
			args[j]->p       = p_pool[j];
			pthread_create(&tid[j], NULL, munmap_only_bench, args[j]);
		}

		for (int j = 0; j < i; j++) {
			pthread_join(tid[j], NULL);
			free(args[j]);
		}
	}

	free(results);
	free(results2);

	return 0;
}
