#
# MIT License
# 
# Copyright (c) 2026 Marco Lizza
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -O2
BENCH_CFLAGS ?= -O3

COMMON_CFLAGS := -std=c99 -Wall -Wextra -Wpedantic
INCLUDES := -Isrc -Isrc/bench -Isrc/test
BUILD_DIR := build

BENCH_SOURCES := \
	src/bench/main.c \
	src/bench/common.c \
	src/bench/scalar.c \
	src/bench/line.c \
	src/bench/sprite.c \
	src/bench/rotoscale.c

BENCH_HEADERS := \
	src/bench/common.h \
	src/fix32.h

TEST_SOURCES := \
	src/test/main.c \
	src/test/conversion.c \
	src/test/arithmetic.c \
	src/test/rounding.c

TEST_HEADERS := \
	src/test/test.h \
	src/fix32.h

BENCHMARK := $(BUILD_DIR)/benchmark
SHIFT_MUL_BENCHMARK := $(BUILD_DIR)/benchmark-shift-mul
SHIFT_DIV_BENCHMARK := $(BUILD_DIR)/benchmark-shift-div
TEST_RUNNER := $(BUILD_DIR)/test
PORTABLE_TEST_RUNNER := $(BUILD_DIR)/test-portable-floor
NO_ROUNDING_TEST_RUNNER := $(BUILD_DIR)/test-no-rounding
MUL32_TEST_RUNNER := $(BUILD_DIR)/test-32-bit-mul
SHIFT_MUL_TEST_RUNNER := $(BUILD_DIR)/test-shift-mul
SHIFT_DIV_TEST_RUNNER := $(BUILD_DIR)/test-shift-div

.PHONY: all benchmark benchmark-shift-mul benchmark-shift-div run-benchmark \
	run-benchmark-shift-mul run-benchmark-shift-div test check clean

all: $(TEST_RUNNER) $(PORTABLE_TEST_RUNNER) $(NO_ROUNDING_TEST_RUNNER) \
	$(MUL32_TEST_RUNNER) $(SHIFT_MUL_TEST_RUNNER) $(SHIFT_DIV_TEST_RUNNER) \
	$(BENCHMARK) $(SHIFT_MUL_BENCHMARK) $(SHIFT_DIV_BENCHMARK)

benchmark: $(BENCHMARK)

benchmark-shift-mul: $(SHIFT_MUL_BENCHMARK)

benchmark-shift-div: $(SHIFT_DIV_BENCHMARK)

run-benchmark: $(BENCHMARK)
	$(BENCHMARK) $(BENCH_ARGS)

run-benchmark-shift-mul: $(SHIFT_MUL_BENCHMARK)
	$(SHIFT_MUL_BENCHMARK) $(BENCH_ARGS)

run-benchmark-shift-div: $(SHIFT_DIV_BENCHMARK)
	$(SHIFT_DIV_BENCHMARK) $(BENCH_ARGS)

test check: $(TEST_RUNNER) $(PORTABLE_TEST_RUNNER) $(NO_ROUNDING_TEST_RUNNER) \
	$(MUL32_TEST_RUNNER) $(SHIFT_MUL_TEST_RUNNER) $(SHIFT_DIV_TEST_RUNNER)
	$(TEST_RUNNER)
	$(PORTABLE_TEST_RUNNER)
	$(NO_ROUNDING_TEST_RUNNER)
	$(MUL32_TEST_RUNNER)
	$(SHIFT_MUL_TEST_RUNNER)
	$(SHIFT_DIV_TEST_RUNNER)

$(BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) $(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_MUL_BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) -DFIX32_USE_SIGNED_SHIFT_MUL=1 \
		$(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_DIV_BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) -DFIX32_USE_SIGNED_SHIFT_DIV=1 \
		$(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(PORTABLE_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_USE_ARITHMETIC_SHIFT_FLOOR=0 $(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(NO_ROUNDING_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_NO_ROUNDING $(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(MUL32_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_FRACTIONAL_BITS=8 -DFIX32_INTEGER_BITS=4 \
		-DTEST_EXPECT_USE_64_BIT=0 $(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_MUL_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_USE_SIGNED_SHIFT_MUL=1 \
		-DTEST_EXPECT_SIGNED_SHIFT_MUL=1 \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_DIV_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_USE_SIGNED_SHIFT_DIV=1 \
		-DTEST_EXPECT_SIGNED_SHIFT_DIV=1 \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

clean:
	$(RM) -r $(BUILD_DIR)
