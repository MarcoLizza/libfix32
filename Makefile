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

TEST_SOURCES := \
	src/test/main.c \
	src/test/conversion.c \
	src/test/arithmetic.c \
	src/test/rounding.c

BENCHMARK := $(BUILD_DIR)/benchmark
TEST_RUNNER := $(BUILD_DIR)/test
PORTABLE_TEST_RUNNER := $(BUILD_DIR)/test-portable-floor

.PHONY: all benchmark run-benchmark test check clean

all: $(TEST_RUNNER) $(PORTABLE_TEST_RUNNER) $(BENCHMARK)

benchmark: $(BENCHMARK)

run-benchmark: $(BENCHMARK)
	$(BENCHMARK) $(BENCH_ARGS)

test check: $(TEST_RUNNER) $(PORTABLE_TEST_RUNNER)
	$(TEST_RUNNER)
	$(PORTABLE_TEST_RUNNER)

$(BENCHMARK): $(BENCH_SOURCES) src/bench/common.h src/fpmath.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(BENCH_CFLAGS) $(BENCH_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(TEST_RUNNER): $(TEST_SOURCES) src/test/test.h src/fpmath.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		$(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

$(PORTABLE_TEST_RUNNER): $(TEST_SOURCES) src/test/test.h src/fpmath.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(CFLAGS) \
		-DFIX32_USE_ARITHMETIC_SHIFT_FLOOR=0 $(TEST_SOURCES) $(LDFLAGS) -o $@ $(LDLIBS)

clean:
	$(RM) -r $(BUILD_DIR)
