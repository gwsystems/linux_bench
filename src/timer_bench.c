/* SPDX-License-Identifier: BSD-2-CLAUSE */

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include "pmu.h"
#include "priority.h"
#include "utils.h"

timer_t          timer;
cycle_t *        results;
int              pp1[2], pp2[2];
int              fd;
pthread_t        t1, t2;
volatile cycle_t global_ts;
volatile int     count;

void
mask_sig(int which, int how)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, which);

	pthread_sigmask(how, &mask, NULL);
}

void
sig_handler()
{
	cycle_t t;

	t = global_ts;
	CHECK(write(pp1[1], &t, sizeof(cycle_t)));
}

void *
bench_thread(void *p)
{
	cycle_t           t, tt;
	struct itimerspec ts;
	struct sigaction  sig_action;
	struct sigevent   evp;

	pthread_prio(SCHED_RR, t1, 0);

	mask_sig(SIGUSR1, SIG_UNBLOCK);

	sig_action.sa_handler = sig_handler;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	CHECK(sigaction(SIGUSR1, &sig_action, NULL));

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify          = SIGEV_SIGNAL;
	evp.sigev_signo           = SIGUSR1;

	CHECK(timer_create(CLOCK_REALTIME, &evp, &timer));
	ts.it_interval.tv_sec  = 0;
	ts.it_interval.tv_nsec = 300000;
	ts.it_value.tv_sec     = 1;
	ts.it_value.tv_nsec    = 0;
	CHECK(timer_settime(timer, 0, &ts, NULL));

	write(pp2[1], &t, sizeof(cycle_t));

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(pp1[0], &t, sizeof(cycle_t)));
		tt = get_cyclecount();

		if (tt > t) { results[i] = tt - t; }
		count--;
	}

	timer_delete(timer);

	return NULL;
}

void *
bench_ep_thread(void *p)
{
	int                epfd;
	cycle_t            t, tt;
	struct epoll_event ev;
	struct itimerspec  ts;
	struct sigaction   sig_action;
	struct sigevent    evp;

	pthread_prio(SCHED_RR, t1, 0);

	mask_sig(SIGUSR1, SIG_UNBLOCK);

	sig_action.sa_handler = sig_handler;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;
	CHECK(sigaction(SIGUSR1, &sig_action, NULL));

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify          = SIGEV_SIGNAL;
	evp.sigev_signo           = SIGUSR1;

	CHECK(timer_create(CLOCK_REALTIME, &evp, &timer));
	ts.it_interval.tv_sec  = 0;
	ts.it_interval.tv_nsec = 300000;
	ts.it_value.tv_sec     = 1;
	ts.it_value.tv_nsec    = 0;
	CHECK(timer_settime(timer, 0, &ts, NULL));

	epfd      = epoll_create(1);
	ev.events = EPOLLIN | EPOLLET;
	CHECK(epoll_ctl(epfd, EPOLL_CTL_ADD, pp1[0], &ev));

	CHECK(write(pp2[1], &t, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		epoll_wait(epfd, &ev, ITERATION, -1);
		tt = get_cyclecount();

		CHECK(read(pp1[0], &t, sizeof(cycle_t)));
		if (tt > t) { results[i] = tt - t; }
		count--;
	}

	timer_delete(timer);

	return NULL;
}

void *
bench_fd_thread(void *p)
{
	cycle_t           t;
	struct itimerspec ts;
	uint64_t          tmp;

	pthread_prio(SCHED_RR, t1, 0);

	fd                     = timerfd_create(CLOCK_REALTIME, 0);
	ts.it_interval.tv_sec  = 0;
	ts.it_interval.tv_nsec = 300000;
	ts.it_value.tv_sec     = 1;
	ts.it_value.tv_nsec    = 0;
	CHECK(timerfd_settime(fd, 0, &ts, NULL));

	CHECK(write(pp2[1], &t, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		CHECK(read(fd, &tmp, sizeof(uint64_t)));
		t = get_cyclecount();

		if (t > global_ts) { results[i] = t - global_ts; }
		count--;
	}

	return NULL;
}

void *
bench_fd_ep_thread(void *p)
{
	int                epfd;
	cycle_t            t;
	struct itimerspec  ts;
	struct epoll_event ev;
	uint64_t           tmp;

	pthread_prio(SCHED_RR, t1, 0);

	fd                     = timerfd_create(CLOCK_REALTIME, 0);
	ts.it_interval.tv_sec  = 0;
	ts.it_interval.tv_nsec = 300000;
	ts.it_value.tv_sec     = 1;
	ts.it_value.tv_nsec    = 0;
	CHECK(timerfd_settime(fd, 0, &ts, NULL));

	epfd      = epoll_create(1);
	ev.events = EPOLLIN | EPOLLET;
	CHECK(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev));

	CHECK(write(pp2[1], &t, sizeof(cycle_t)));

	for (int i = 0; i < ITERATION; i++) {
		epoll_wait(epfd, &ev, ITERATION, -1);
		t = get_cyclecount();

		if (t > global_ts) { results[i] = t - global_ts; }
		CHECK(read(fd, &tmp, sizeof(uint64_t)));
		count--;
	}

	return NULL;
}

void *
loop_thread(void *p)
{
	cycle_t t;

	pthread_prio(SCHED_RR, t2, 1);

	CHECK(read(pp2[0], &t, sizeof(cycle_t)));

	count = ITERATION;
	while (count > 0) { global_ts = get_cyclecount(); }

	return NULL;
}

void
timer_bench()
{
	pipe(pp1);
	pipe(pp2);

	mask_sig(SIGUSR1, SIG_BLOCK);

	utils_clean_results(results);

	pthread_create(&t1, NULL, bench_thread, NULL);
	pthread_create(&t2, NULL, loop_thread, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench13-timer+signal+read");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("timer + signal + read", results);
	utils_store_header("#####################EndBench13-timer+signal+read");

	utils_clean_results(results);

	pthread_create(&t1, NULL, bench_ep_thread, NULL);
	pthread_create(&t2, NULL, loop_thread, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench14-timer+signal+epoll");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("timer + signal + read", results);
	utils_store_header("#####################EndBench14-timer+signal+epoll");

	utils_clean_results(results);

	pthread_create(&t1, NULL, bench_fd_thread, NULL);
	pthread_create(&t2, NULL, loop_thread, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench15-timerfd+read");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("timerfd + read", results);
	utils_store_header("#####################EndBench15-timerfd+read");

	utils_clean_results(results);

	pthread_create(&t1, NULL, bench_fd_ep_thread, NULL);
	pthread_create(&t2, NULL, loop_thread, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	utils_store_header("#####################BeginBench16-timerfd+epoll");
	utils_store_header("#Latency");
	utils_store_results(results);
	utils_print_summary("timerfd + epoll", results);
	utils_store_header("#####################EndBench16-timerfd+epoll");
}


int
main(int argc, char *argv[])
{
	set_affinity();
	set_prio(SCHED_RR, 2);

	results = malloc(sizeof(cycle_t) * ITERATION);

	init_perfcounters(1, 0);

	timer_bench();

	free(results);

	return 0;
}
