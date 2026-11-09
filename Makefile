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
.DEFAULT_GOAL := all
CPPFLAGS ?=
CFLAGS ?= -O2
BENCH_CFLAGS ?= -O3

COMMON_CFLAGS := -std=c99 -Wall -Wextra -Wpedantic
BENCH_CPPFLAGS ?= -DFIX32_STATIC_INLINE
INCLUDES := -Isrc -Isrc/bench -Isrc/test
BUILD_DIR := build

BENCH_SOURCES := \
	src/bench/main.c \
	src/bench/common.c \
	src/bench/scalar.c \
	src/bench/multiply.c \
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
	src/test/rounding.c \
	src/test/boundary.c

TEST_HEADERS := \
	src/test/test.h \
	src/fix32.h

BENCHMARK := $(BUILD_DIR)/benchmark
NARROW_BENCHMARK := $(BUILD_DIR)/benchmark-32-bit
SHIFT_MUL_BENCHMARK := $(BUILD_DIR)/benchmark-shift-mul
SHIFT_DIV_BENCHMARK := $(BUILD_DIR)/benchmark-shift-div
BENCH_OUTPUT ?= $(BUILD_DIR)/benchmark-output.txt
BENCH_GRAPH ?= docs/benchmark_summary.svg
BENCH_GRAPH_SCRIPT := tools/benchmark_graph.py
PYTHON ?= python3
TEST_RUNNER := $(BUILD_DIR)/test
PORTABLE_TEST_RUNNER := $(BUILD_DIR)/test-portable-floor
NO_ROUNDING_TEST_RUNNER := $(BUILD_DIR)/test-no-rounding
MUL32_TEST_RUNNER := $(BUILD_DIR)/test-32-bit-mul
SHIFT_MUL_TEST_RUNNER := $(BUILD_DIR)/test-shift-mul
SHIFT_DIV_TEST_RUNNER := $(BUILD_DIR)/test-shift-div
SHIFT_DIV32_TEST_RUNNER := $(BUILD_DIR)/test-shift-div-32-bit
INLINE_TEST_RUNNER := $(BUILD_DIR)/test-static-inline
BENCH_DATA_TEST_RUNNER := $(BUILD_DIR)/test-benchmark-data
FRACTIONAL_BITS := 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 \
	16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
BOUNDARY_TEST_RUNNERS := $(addprefix $(BUILD_DIR)/test-boundary-,$(FRACTIONAL_BITS)) \
	$(addprefix $(BUILD_DIR)/test-boundary-narrow-,$(FRACTIONAL_BITS))
TEST_RUNNERS := $(TEST_RUNNER) $(PORTABLE_TEST_RUNNER) $(NO_ROUNDING_TEST_RUNNER) \
	$(MUL32_TEST_RUNNER) $(SHIFT_MUL_TEST_RUNNER) \
	$(SHIFT_DIV_TEST_RUNNER) $(SHIFT_DIV32_TEST_RUNNER) $(INLINE_TEST_RUNNER) \
	$(BENCH_DATA_TEST_RUNNER) $(BOUNDARY_TEST_RUNNERS)

# GNU make does not otherwise track command-line flag changes. Writing this
# file only when its contents change preserves normal incremental builds.
define BUILD_FLAGS
CC=$(CC)
CPPFLAGS=$(CPPFLAGS)
CFLAGS=$(CFLAGS)
COMMON_CFLAGS=$(COMMON_CFLAGS)
INCLUDES=$(INCLUDES)
BENCH_CPPFLAGS=$(BENCH_CPPFLAGS)
BENCH_CFLAGS=$(BENCH_CFLAGS)
LDFLAGS=$(LDFLAGS)
LDLIBS=$(LDLIBS)
endef

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/.build-flags: FORCE | $(BUILD_DIR)
	$(file >$@.tmp,$(BUILD_FLAGS))
	@if ! cmp -s $@.tmp $@; then cp $@.tmp $@; fi
	@$(RM) $@.tmp

$(TEST_RUNNERS) $(BENCHMARK) $(NARROW_BENCHMARK) $(SHIFT_MUL_BENCHMARK) \
	$(SHIFT_DIV_BENCHMARK): \
	$(BUILD_DIR)/.build-flags Makefile

.PHONY: all benchmark benchmark-32-bit benchmark-shift-mul \
	benchmark-shift-div run-benchmark run-benchmark-32-bit \
	run-benchmark-shift-mul run-benchmark-shift-div \
	test check clean FORCE

all: $(TEST_RUNNERS) $(BENCHMARK) $(NARROW_BENCHMARK) $(SHIFT_MUL_BENCHMARK) \
	$(SHIFT_DIV_BENCHMARK)

benchmark: $(BENCHMARK)

benchmark-32-bit: $(NARROW_BENCHMARK)

benchmark-shift-mul: $(SHIFT_MUL_BENCHMARK)

benchmark-shift-div: $(SHIFT_DIV_BENCHMARK)

run-benchmark: $(BENCHMARK) $(BENCH_GRAPH_SCRIPT)
	@mkdir -p $(BUILD_DIR)
	$(BENCHMARK) $(BENCH_ARGS) > $(BENCH_OUTPUT)
	@cat $(BENCH_OUTPUT)
	$(PYTHON) $(BENCH_GRAPH_SCRIPT) $(BENCH_OUTPUT) -o $(BENCH_GRAPH) \
		--source-label "$(BENCH_OUTPUT)"

run-benchmark-shift-mul: $(SHIFT_MUL_BENCHMARK)
	$(SHIFT_MUL_BENCHMARK) $(BENCH_ARGS)

run-benchmark-shift-div: $(SHIFT_DIV_BENCHMARK)
	$(SHIFT_DIV_BENCHMARK) $(BENCH_ARGS)

run-benchmark-32-bit: $(NARROW_BENCHMARK)
	$(NARROW_BENCHMARK) $(BENCH_ARGS)

test check: $(TEST_RUNNERS)
	@set -e; for runner in $(TEST_RUNNERS); do "$$runner"; done

$(BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BENCH_CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) $(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(NARROW_BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BENCH_CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) -DFIX32_FRACTIONAL_BITS=8 \
		-DFIX32_USE_64_BIT=0 -DBENCHMARK_NARROW_INPUTS=1 \
		$(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_MUL_BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BENCH_CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) -DFIX32_USE_SIGNED_SHIFT_MUL=1 \
		$(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_DIV_BENCHMARK): $(BENCH_SOURCES) $(BENCH_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BENCH_CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
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

$(INLINE_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_STATIC_INLINE \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_DIV_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_USE_SIGNED_SHIFT_DIV=1 -DTEST_EXPECT_SIGNED_SHIFT_DIV=1 \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(SHIFT_DIV32_TEST_RUNNER): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_FRACTIONAL_BITS=8 -DFIX32_USE_64_BIT=0 -DTEST_EXPECT_USE_64_BIT=0 \
		-DFIX32_USE_SIGNED_SHIFT_DIV=1 -DTEST_EXPECT_SIGNED_SHIFT_DIV=1 \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(BUILD_DIR)/test-boundary-%: src/test/main.c src/test/boundary.c $(TEST_HEADERS)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DTEST_BOUNDARIES_ONLY -DFIX32_FRACTIONAL_BITS=$* \
		src/test/main.c src/test/boundary.c $(LDFLAGS) -o $@ $(LDLIBS)

$(BUILD_DIR)/test-boundary-narrow-%: src/test/main.c src/test/boundary.c $(TEST_HEADERS)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DTEST_BOUNDARIES_ONLY -DFIX32_FRACTIONAL_BITS=$* -DFIX32_USE_64_BIT=0 \
		src/test/main.c src/test/boundary.c $(LDFLAGS) -o $@ $(LDLIBS)

$(BENCH_DATA_TEST_RUNNER): src/test/benchmark.c src/bench/common.c $(BENCH_HEADERS)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		src/test/benchmark.c src/bench/common.c $(LDFLAGS) -o $@ $(LDLIBS)

clean:
	$(RM) -r $(BUILD_DIR)
