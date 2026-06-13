/*
 * MIT License
 *
 * Copyright (c) 2026 Marco Lizza
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FPMATH_BENCH_COMMON_H
#define FPMATH_BENCH_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "fpmath.h"

#define DEFAULT_SAMPLE_COUNT 16384u
#define DEFAULT_REPEAT_COUNT 2000u

typedef struct benchmark_data {
    size_t sample_count;
    size_t repeat_count;
    int32_t *int_inputs;
    int32_t *small_int_inputs;
    int32_t *int_div_inputs;
    float *float_inputs;
    double *double_inputs;
    float *sum_inputs;
    float *div_inputs;
    float *float_reciprocals;
    fix32_t *fixed_inputs;
    fix32_t *sum_fixed_inputs;
    fix32_t *fixed_div_inputs;
    fix32_t *fixed_reciprocals;
} benchmark_data_t;

extern volatile int64_t g_int_sink;
extern volatile double g_float_sink;

int benchmark_data_init(benchmark_data_t *data, size_t sample_count,
                        size_t repeat_count);
void benchmark_data_destroy(benchmark_data_t *data);
int benchmark_parse_size_arg(const char *text, size_t *value);

static inline int32_t benchmark_int_abs(int32_t value)
{
    return (value < 0) ? -value : value;
}

static inline int32_t benchmark_int_max(int32_t left, int32_t right)
{
    return (left > right) ? left : right;
}

static inline int32_t benchmark_float_floor_to_int(float value)
{
    const int32_t whole = (int32_t)value;

    return ((float)whole > value) ? whole - 1 : whole;
}

static inline int32_t benchmark_float_ceil_to_int(float value)
{
    const int32_t whole = (int32_t)value;

    return ((float)whole < value) ? whole + 1 : whole;
}

static inline int32_t benchmark_float_round_to_int(float value)
{
    const int32_t whole = (int32_t)value;
    const float fractional = value - (float)whole;

    if (fractional >= 0.5f) {
        return whole + 1;
    }
    if (fractional <= -0.5f) {
        return whole - 1;
    }
    return whole;
}

static inline float benchmark_float_floor_to_float(float value)
{
    return (float)benchmark_float_floor_to_int(value);
}

static inline float benchmark_float_ceil_to_float(float value)
{
    return (float)benchmark_float_ceil_to_int(value);
}

static inline float benchmark_float_round_to_float(float value)
{
    return (float)benchmark_float_round_to_int(value);
}

void benchmark_print_scalar_results(const benchmark_data_t *data);
void benchmark_print_line_results(size_t repeat_count);
void benchmark_print_sprite_results(size_t repeat_count);
void benchmark_print_rotoscale_results(size_t repeat_count);
void benchmark_print_rotoscale_direct_results(size_t repeat_count);

#endif
