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

#include "common.h"

#include <stdio.h>

static void fixed_array(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    const fix32_t *restrict left = data->fixed_inputs;
    const fix32_t *restrict right = data->sum_fixed_inputs;
    fix32_t *restrict output = data->fixed_results;
    size_t index;

    (void)repeat_count;
    for (index = 0; index < data->sample_count; ++index) {
        output[index] = fix32_mul(left[index], right[index]);
    }
}

static void float_array(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    const float *restrict left = data->float_inputs;
    const float *restrict right = data->sum_inputs;
    float *restrict output = data->float_results;
    size_t index;

    (void)repeat_count;
    for (index = 0; index < data->sample_count; ++index) {
        output[index] = left[index] * right[index];
    }
}

static void bench_fixed_array(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    // An indirect call through a volatile pointer keeps every batch observable
    // even with LTO, without volatile loads in the vectorizable array kernel.
    benchmark_fn volatile kernel = fixed_array;
    double checksum = 0.0;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        kernel(context, 1);
    }
    for (index = 0; index < data->sample_count; ++index) {
        checksum += data->fixed_results[index];
    }
    g_float_sink = checksum;
}

static void bench_float_array(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    benchmark_fn volatile kernel = float_array;
    double checksum = 0.0;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        kernel(context, 1);
    }
    for (index = 0; index < data->sample_count; ++index) {
        checksum += data->float_results[index];
    }
    g_float_sink = checksum;
}

static void bench_fixed_chain(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    fix32_t value = FIX32_ONE;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            value = fix32_add(fix32_mul(value, data->sum_fixed_inputs[index]),
                              FIX32_ONE);
        }
    }
    g_float_sink = (double)value;
}

static void bench_float_chain(const void *context, size_t repeat_count)
{
    const benchmark_data_t *data = context;
    float value = 1.0f;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            value = (value * data->sum_inputs[index]) + 1.0f;
        }
    }
    g_float_sink = (double)value;
}

void benchmark_print_multiply_results(const benchmark_data_t *data)
{
    static const char *labels[] = { "multiply array", "multiply/add chain" };
    const benchmark_fn fixed_functions[] = { bench_fixed_array, bench_fixed_chain };
    const benchmark_fn float_functions[] = { bench_float_array, bench_float_chain };
    const double operations = (double)data->sample_count * data->repeat_count;
    size_t index;

    printf("\nmultiply throughput and dependency benchmarks\n");
    printf("%-24s %14s %14s %12s\n",
           "operation", "fixed ns/op", "base ns/op", "fixed/base");
    for (index = 0; index < sizeof(labels) / sizeof(labels[0]); ++index) {
        const benchmark_measurement_t measurement =
            benchmark_measure_pair(fixed_functions[index], float_functions[index],
                                   data, data->repeat_count);
        const double fixed_ns = measurement.fixed_seconds * 1.0e9 / operations;
        const double float_ns = measurement.float_seconds * 1.0e9 / operations;
        const double ratio = (float_ns > 0.0) ? fixed_ns / float_ns : 0.0;

        printf("%-24s %14.2f %14.2f %12.2f\n",
               labels[index], fixed_ns, float_ns, ratio);
        benchmark_print_spread(&measurement);
    }
}
