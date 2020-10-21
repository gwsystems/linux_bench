/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
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

void
reader_ep_work()
{
	int                epfd;
	cycle_t            ts;
	struct epoll_event ev;

	epfd      = epoll_create(1);
	ev.events = EPOLLIN | EPOLLET;
	CHECK(epoll_ctl(epfd, EPOLL_CTL_ADD, pp[0], &ev));

	CHECK(read(pp3[0], &ts, sizeof(cycle_t)));
	CHECK(write(pp4[1], &ts, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		epoll_wait(epfd, &ev, ITERATION, -1);
		CHECK(read(pp[0], &ts, sizeof(cycle_t)));
		ts = get_cyclecount();
		CHECK(write(pp2[1], &ts, sizeof(cycle_t)));
	}
}

void
writer_ep_work()
{
	int                epfd;
	cycle_t            ts, ts2, ts3;
	struct epoll_event ev;

	epfd      = epoll_create(1);
	ev.events = EPOLLIN | EPOLLET;
	CHECK(epoll_ctl(epfd, EPOLL_CTL_ADD, pp2[0], &ev));

	CHECK(write(pp3[1], &ts, sizeof(cycle_t)));
	CHECK(read(pp4[0], &ts, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		ts = get_cyclecount();
		CHECK(write(pp[1], &ts, sizeof(cycle_t)));
		epoll_wait(epfd, &ev, ITERATION, -1);
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

void *
reader_ep_thread(void *p)
{
	pthread_prio(SCHED_RR, t1, *(int *)p);

	reader_ep_work();

	return NULL;
}

void *
writer_ep_thread(void *p)
{
	pthread_prio(SCHED_RR, t2, *(int *)p);

	writer_ep_work();

	return NULL;
}

void
bench_processes()
{
	pid_t p;
	int   status;

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

	utils_store_header("#####################BeginBench04-pipe-processes-oneway-reader-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("pipe + between processes + oneway + reader high prio", results);
	utils_store_header("#####################EndBench04-pipe-processes-oneway-reader-high-prio");

	utils_store_header("#####################BeginBench05-pipe-processes-oneway-writer-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results2);
	utils_print_summary("pipe + between processes + oneway + writer high prio", results2);
	utils_store_header("#####################EndBench05-pipe-processes-oneway-writer-high-prio");

	utils_store_header("#####################BeginBench06-pipe-processes-roundtrip");
	utils_store_header("#Latency");
	utils_store_results(results3);
	utils_print_summary("pipe + between processes + roundtrip", results3);
	utils_store_header("#####################EndBench06-pipe-processes-roundtrip");

	utils_clean_results(results);
	utils_clean_results(results2);
	utils_clean_results(results3);

	p = fork();
	if (p == 0) {
		set_prio(SCHED_RR, 0);
		reader_ep_work();

		exit(EXIT_SUCCESS);
	} else {
		set_prio(SCHED_RR, 1);
		writer_ep_work();
	}

	waitpid(p, &status, 0);

	utils_store_header("#####################BeginBench22-pipe-processes-ep-oneway-reader-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("pipe + between processes + epoll + oneway + reader high prio", results);
	utils_store_header("#####################EndBench22-pipe-processes-ep-oneway-reader-high-prio");

	utils_store_header("#####################BeginBench23-pipe-processes-ep-oneway-writer-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results2);
	utils_print_summary("pipe + between processes + epoll + oneway + writer high prio", results2);
	utils_store_header("#####################EndBench23-pipe-processes-ep-oneway-writer-high-prio");

	utils_store_header("#####################BeginBench24-pipe-processes-ep-roundtrip");
	utils_store_header("#Latency");
	utils_store_results(results3);
	utils_print_summary("pipe + between processes + epoll + roundtrip", results3);
	utils_store_header("#####################EndBench24-pipe-processes-ep-roundtrip");
}

void
bench_threads()
{
	int n1, n2;

	utils_clean_results(results);
	utils_clean_results(results2);
	utils_clean_results(results3);

	n1 = 0;
	n2 = 1;
	pthread_create(&t1, NULL, reader_thread, &n1);
	pthread_create(&t2, NULL, writer_thread, &n2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench01-pipe-threads-oneway-reader-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("pipe + between threads + oneway + reader high prio", results);
	utils_store_header("#####################EndBench01-pipe-threads-oneway-reader-high-prio");

	utils_store_header("#####################BeginBench02-pipe-threads-oneway-writer-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results2);
	utils_print_summary("pipe + between threads + oneway + writer high prio", results2);
	utils_store_header("#####################EndBench02-pipe-threads-oneway-writer-high-prio");

	utils_store_header("#####################BeginBench03-pipe-threads-roundtrip");
	utils_store_header("#Latency");
	utils_store_results(results3);
	utils_print_summary("pipe + between threads + roundtrip", results3);
	utils_store_header("#####################EndBench03-pipe-threads-roundtrip");

	utils_clean_results(results);
	utils_clean_results(results2);
	utils_clean_results(results3);

	n1 = 0;
	n2 = 1;
	pthread_create(&t1, NULL, reader_ep_thread, &n1);
	pthread_create(&t2, NULL, writer_ep_thread, &n2);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench19-pipe-threads-ep-oneway-reader-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("pipe + between threads + epoll + oneway + reader high prio", results);
	utils_store_header("#####################EndBench19-pipe-threads-ep-oneway-reader-high-prio");

	utils_store_header("#####################BeginBench20-pipe-threads-ep-oneway-writer-high-prio");
	utils_store_header("#Latency");
	utils_store_results(results2);
	utils_print_summary("pipe + between threads + epoll + oneway + writer high prio", results2);
	utils_store_header("#####################EndBench20-pipe-threads-ep-oneway-writer-high-prio");

	utils_store_header("#####################BeginBench21-pipe-threads-ep-roundtrip");
	utils_store_header("#Latency");
	utils_store_results(results3);
	utils_print_summary("pipe + between threads + epoll + roundtrip", results3);
	utils_store_header("#####################EndBench21-pipe-threads-ep-roundtrip");
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

	bench_threads();
	bench_processes();

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
