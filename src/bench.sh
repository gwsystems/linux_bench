#!/bin/sh

echo -1 > /proc/sys/kernel/sched_rt_runtime_us

rm -rf bench_results

./pipe_bench
./mutex_bench
./semaphore_bench
./timer_bench
./cs_bench
