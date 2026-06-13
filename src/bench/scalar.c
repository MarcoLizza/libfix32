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
#include <time.h>

typedef void (*benchmark_fn)(const benchmark_data_t *data);

typedef struct benchmark_pair {
    const char *label;
    benchmark_fn fixed_fn;
    benchmark_fn float_fn;
} benchmark_pair_t;

static double run_benchmark(benchmark_fn fn, const benchmark_data_t *data)
{
    const clock_t start = clock();
    const double ticks_per_second = (double)CLOCKS_PER_SEC;
    clock_t finish;

    fn(data);
    finish = clock();

    return (double)(finish - start) / ticks_per_second;
}

static void bench_fixed_from_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const int32_t *inputs = data->int_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t value = fix32_from_int(inputs[index]);
            checksum += value;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_from_int(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const int32_t *inputs = data->int_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const float value = (float)inputs[index];
            checksum += value;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_from_float(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t value = fix32_round_from_float(inputs[index]);
            checksum += value;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_from_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const float value = inputs[index];
            checksum += value;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_sum(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *inputs = data->sum_fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        fix32_t total = 0;

        for (index = 0; index < data->sample_count; ++index) {
            total = fix32_add(total, inputs[index]);
        }

        checksum += total;
    }

    g_int_sink = checksum;
}

static void bench_float_sum(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->sum_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        float total = 0.0f;

        for (index = 0; index < data->sample_count; ++index) {
            total += inputs[index];
        }

        checksum += total;
    }

    g_float_sink = checksum;
}

static void bench_fixed_mul(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *left_inputs = data->fixed_inputs;
    volatile const fix32_t *right_inputs = data->sum_fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t product =
                fix32_mul(left_inputs[index], right_inputs[index]);
            checksum += product;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_mul(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *left_inputs = data->float_inputs;
    volatile const float *right_inputs = data->sum_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const float product = left_inputs[index] * right_inputs[index];
            checksum += product;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_reciprocal(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *inputs = data->fixed_div_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t reciprocal = fix32_reciprocal(inputs[index]);
            checksum += reciprocal;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_reciprocal(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const float *inputs = data->div_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t reciprocal =
                fix32_round_from_float(1.0f / inputs[index]);
            checksum += reciprocal;
        }
    }

    g_int_sink = checksum;
}

static void bench_fixed_div(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *left_inputs = data->fixed_inputs;
    volatile const fix32_t *divisor_inputs = data->fixed_div_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t quotient =
                fix32_div(left_inputs[index], divisor_inputs[index]);
            checksum += quotient;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_div(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *left_inputs = data->float_inputs;
    volatile const float *divisor_inputs = data->div_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const float quotient = left_inputs[index] / divisor_inputs[index];
            checksum += quotient;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_mul_reciprocal(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *left_inputs = data->fixed_inputs;
    volatile const fix32_t *reciprocal_inputs = data->fixed_reciprocals;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const fix32_t product =
                fix32_mul(left_inputs[index], reciprocal_inputs[index]);
            checksum += product;
        }
    }

    g_int_sink = checksum;
}

static void bench_float_mul_reciprocal(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *left_inputs = data->float_inputs;
    volatile const float *reciprocal_inputs = data->float_reciprocals;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            const float product = left_inputs[index] * reciprocal_inputs[index];
            checksum += product;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_ceil_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_ceil_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_fixed_floor_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_floor_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_float_ceil_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_ceil_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_float_floor_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_floor_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_fixed_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_ceil_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_ceil_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_floor_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_floor_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_float_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += inputs[index];
        }
    }

    g_float_sink = checksum;
}

static void bench_float_ceil_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_ceil_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_float_floor_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_floor_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_round_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_round_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_fixed_round_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const fix32_t *inputs = data->fixed_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += fix32_round_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_float_round_to_int(const benchmark_data_t *data)
{
    int64_t checksum = 0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_round_to_int(inputs[index]);
        }
    }

    g_int_sink = checksum;
}

static void bench_float_round_to_float(const benchmark_data_t *data)
{
    double checksum = 0.0;
    volatile const float *inputs = data->float_inputs;
    size_t repeat;
    size_t index;

    for (repeat = 0; repeat < data->repeat_count; ++repeat) {
        for (index = 0; index < data->sample_count; ++index) {
            checksum += benchmark_float_round_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

void benchmark_print_scalar_results(const benchmark_data_t *data)
{
    static const benchmark_pair_t pairs[] = {
        { "int -> representation", bench_fixed_from_int, bench_float_from_int },
        { "float -> representation", bench_fixed_from_float,
          bench_float_from_float },
        { "sum", bench_fixed_sum, bench_float_sum },
        { "multiply", bench_fixed_mul, bench_float_mul },
        { "reciprocal -> fixed", bench_fixed_reciprocal,
          bench_float_reciprocal },
        { "divide", bench_fixed_div, bench_float_div },
        { "mul reciprocal", bench_fixed_mul_reciprocal,
          bench_float_mul_reciprocal },
        { "ceil -> int", bench_fixed_ceil_to_int, bench_float_ceil_to_int },
        { "floor -> int", bench_fixed_floor_to_int, bench_float_floor_to_int },
        { "round -> int", bench_fixed_round_to_int, bench_float_round_to_int },
        { "value -> float", bench_fixed_to_float, bench_float_to_float },
        { "ceil -> float", bench_fixed_ceil_to_float,
          bench_float_ceil_to_float },
        { "floor -> float", bench_fixed_floor_to_float,
          bench_float_floor_to_float },
        { "round -> float", bench_fixed_round_to_float,
          bench_float_round_to_float },
    };
    const size_t benchmark_count = sizeof(pairs) / sizeof(pairs[0]);
    const double operations =
        (double)data->sample_count * (double)data->repeat_count;
    size_t index;

    printf("scalar benchmarks\n");
    printf("samples=%lu repeats=%lu operations-per-case=%.0f\n",
           (unsigned long)data->sample_count, (unsigned long)data->repeat_count,
           operations);
    printf("%-24s %14s %14s %12s\n",
           "operation", "fixed ns/op", "float ns/op", "fixed/float");

    for (index = 0; index < benchmark_count; ++index) {
        const double fixed_seconds = run_benchmark(pairs[index].fixed_fn, data);
        const double float_seconds = run_benchmark(pairs[index].float_fn, data);
        const double fixed_ns = (fixed_seconds * 1.0e9) / operations;
        const double float_ns = (float_seconds * 1.0e9) / operations;
        const double ratio = (float_ns > 0.0) ? (fixed_ns / float_ns) : 0.0;

        printf("%-24s %14.2f %14.2f %12.2f\n",
               pairs[index].label, fixed_ns, float_ns, ratio);
    }
}
