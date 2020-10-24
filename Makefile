# SPDX-License-Identifier: BSD-2-CLAUSE

CC = $(CROSS_COMPILE)gcc
CFLAGS = -lpthread -lrt -g -O3 -Wall -Werror
SRC_DIR = src
BUILD_OUTPUT = out
BIN = linux_bench

deps = src/priority.c src/utils.c

all: pipe_bench mutex_bench semaphore_bench timer_bench cs_bench sysc_bench bench_script

pipe_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/pipe_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

mutex_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/mutex_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

semaphore_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/semaphore_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

timer_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/timer_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

cs_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/cs_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

sysc_bench:
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(SRC_DIR)/sysc_bench.c $(deps) -o $(BUILD_OUTPUT)/$@ $(CFLAGS)

bench_script:
	@mkdir -p $(BUILD_OUTPUT)
	cp $(SRC_DIR)/bench.sh $(BUILD_OUTPUT)/

clean:
	@rm -rf $(BUILD_OUTPUT)

format: 
	@clang-format -style=file -i src/*.c
	@clang-format -style=file -i src/*.h

.PHONY: all clean format
