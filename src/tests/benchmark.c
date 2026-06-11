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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef FIX32_ENABLE_DEBUG_CHECKS
#define FIX32_ENABLE_DEBUG_CHECKS 0
#endif
#include "../fpmath.h"

#define DEFAULT_SAMPLE_COUNT 16384u
#define DEFAULT_REPEAT_COUNT 2000u
#define DDA_CANVAS_STRIDE 4099

typedef struct benchmark_data {
    size_t sample_count;
    size_t repeat_count;
    int32_t *int_inputs;
    float *float_inputs;
    float *sum_inputs;
    float *div_inputs;
    float *float_reciprocals;
    fix32_t *fixed_inputs;
    fix32_t *sum_fixed_inputs;
    fix32_t *fixed_div_inputs;
    fix32_t *fixed_reciprocals;
} benchmark_data_t;

typedef void (*benchmark_fn)(const benchmark_data_t *data);

typedef struct benchmark_pair {
    const char *label;
    benchmark_fn fixed_fn;
    benchmark_fn float_fn;
} benchmark_pair_t;

typedef void (*line_benchmark_fn)(size_t line_length, size_t repeat_count);

typedef struct sprite_case {
    const char *label;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_width;
    int32_t dest_height;
} sprite_case_t;

typedef void (*sprite_benchmark_fn)(const sprite_case_t *sprite_case,
                                    size_t repeat_count);

typedef struct rotoscale_case {
    const char *label;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_width;
    int32_t dest_height;
    float cos_angle;
    float sin_angle;
} rotoscale_case_t;

typedef void (*rotoscale_benchmark_fn)(const rotoscale_case_t *rotoscale_case,
                                       size_t repeat_count);

static volatile int64_t g_int_sink = 0;
static volatile double g_float_sink = 0.0;

static uint32_t lcg_next(uint32_t *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static float random_float(uint32_t *state, float minimum, float maximum)
{
    const float unit = (float)(lcg_next(state) & 0x00ffffffu) / 16777215.0f;
    return minimum + (unit * (maximum - minimum));
}

static float random_divisor(uint32_t *state)
{
    const float value = random_float(state, -16.0f, 16.0f);

    return (value >= 0.0f) ? (value + 0.5f) : (value - 0.5f);
}

static void *checked_alloc(size_t count, size_t item_size)
{
    void *memory = malloc(count * item_size);

    if (memory == NULL) {
        fprintf(stderr, "allocation failed for %lu bytes\n",
                (unsigned long)(count * item_size));
    }

    return memory;
}

static int benchmark_data_init(benchmark_data_t *data, size_t sample_count,
                               size_t repeat_count)
{
    uint32_t state = 0x9e3779b9u;
    size_t index;

    memset(data, 0, sizeof(*data));
    data->sample_count = sample_count;
    data->repeat_count = repeat_count;

    data->int_inputs = checked_alloc(sample_count, sizeof(*data->int_inputs));
    data->float_inputs = checked_alloc(sample_count, sizeof(*data->float_inputs));
    data->sum_inputs = checked_alloc(sample_count, sizeof(*data->sum_inputs));
    data->div_inputs = checked_alloc(sample_count, sizeof(*data->div_inputs));
    data->float_reciprocals =
        checked_alloc(sample_count, sizeof(*data->float_reciprocals));
    data->fixed_inputs = checked_alloc(sample_count, sizeof(*data->fixed_inputs));
    data->sum_fixed_inputs =
        checked_alloc(sample_count, sizeof(*data->sum_fixed_inputs));
    data->fixed_div_inputs =
        checked_alloc(sample_count, sizeof(*data->fixed_div_inputs));
    data->fixed_reciprocals =
        checked_alloc(sample_count, sizeof(*data->fixed_reciprocals));

    if (data->int_inputs == NULL || data->float_inputs == NULL ||
        data->sum_inputs == NULL || data->div_inputs == NULL ||
        data->float_reciprocals == NULL || data->fixed_inputs == NULL ||
        data->sum_fixed_inputs == NULL || data->fixed_div_inputs == NULL ||
        data->fixed_reciprocals == NULL) {
        return 0;
    }

    for (index = 0; index < sample_count; ++index) {
        data->int_inputs[index] = (int32_t)(lcg_next(&state) % 60001u) - 30000;
        data->float_inputs[index] = random_float(&state, -256.0f, 256.0f);
        data->sum_inputs[index] = random_float(&state, -0.5f, 0.5f);
        data->div_inputs[index] = random_divisor(&state);
        data->float_reciprocals[index] = 1.0f / data->div_inputs[index];
        data->fixed_inputs[index] = fix32_from_float(data->float_inputs[index]);
        data->sum_fixed_inputs[index] = fix32_from_float(data->sum_inputs[index]);
        data->fixed_div_inputs[index] = fix32_from_float(data->div_inputs[index]);
        data->fixed_reciprocals[index] =
            fix32_reciprocal(data->fixed_div_inputs[index]);
    }

    return 1;
}

static void benchmark_data_destroy(benchmark_data_t *data)
{
    free(data->int_inputs);
    free(data->float_inputs);
    free(data->sum_inputs);
    free(data->div_inputs);
    free(data->float_reciprocals);
    free(data->fixed_inputs);
    free(data->sum_fixed_inputs);
    free(data->fixed_div_inputs);
    free(data->fixed_reciprocals);
}

static int parse_size_arg(const char *text, size_t *value)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0u) {
        return 0;
    }

    *value = (size_t)parsed;
    return 1;
}

static double run_benchmark(benchmark_fn fn, const benchmark_data_t *data)
{
    const clock_t start = clock();
    const double ticks_per_second = (double)CLOCKS_PER_SEC;
    clock_t finish;

    fn(data);
    finish = clock();

    return (double)(finish - start) / ticks_per_second;
}

static double run_line_benchmark(line_benchmark_fn fn, size_t line_length,
                                 size_t repeat_count)
{
    const clock_t start = clock();
    const double ticks_per_second = (double)CLOCKS_PER_SEC;
    clock_t finish;

    fn(line_length, repeat_count);
    finish = clock();

    return (double)(finish - start) / ticks_per_second;
}

static double run_sprite_benchmark(sprite_benchmark_fn fn,
                                   const sprite_case_t *sprite_case,
                                   size_t repeat_count)
{
    const clock_t start = clock();
    const double ticks_per_second = (double)CLOCKS_PER_SEC;
    clock_t finish;

    fn(sprite_case, repeat_count);
    finish = clock();

    return (double)(finish - start) / ticks_per_second;
}

static double run_rotoscale_benchmark(rotoscale_benchmark_fn fn,
                                      const rotoscale_case_t *rotoscale_case,
                                      size_t repeat_count)
{
    const clock_t start = clock();
    const double ticks_per_second = (double)CLOCKS_PER_SEC;
    clock_t finish;

    fn(rotoscale_case, repeat_count);
    finish = clock();

    return (double)(finish - start) / ticks_per_second;
}

static int32_t int_abs(int32_t value)
{
    return (value < 0) ? -value : value;
}

static int32_t int_max(int32_t left, int32_t right)
{
    return (left > right) ? left : right;
}

static void line_endpoints(size_t line_length, int32_t *start_x, int32_t *start_y,
                           int32_t *end_x, int32_t *end_y)
{
    const int32_t dx = (int32_t)line_length;
    const int32_t dy = (int32_t)((line_length * 5u) / 8u) + 1;

    *start_x = -((int32_t)line_length / 3) - 7;
    *start_y = -((int32_t)line_length / 4) - 5;
    *end_x = *start_x + dx;
    *end_y = *start_y + dy;
}

static size_t line_point_count(size_t line_length)
{
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
    line_endpoints(line_length, &start_x, &start_y, &end_x, &end_y);

    return (size_t)(int_max(int_abs(end_x - start_x), int_abs(end_y - start_y)) + 1);
}

static int32_t float_floor_to_int(float value)
{
    const int32_t whole = (int32_t)value;

    if ((float)whole > value) {
        return whole - 1;
    }

    return whole;
}

static int32_t float_ceil_to_int(float value)
{
    const int32_t whole = (int32_t)value;

    if ((float)whole < value) {
        return whole + 1;
    }

    return whole;
}

static int32_t float_round_to_int(float value)
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

static float float_floor_to_float(float value)
{
    return (float)float_floor_to_int(value);
}

static float float_ceil_to_float(float value)
{
    return (float)float_ceil_to_int(value);
}

static float float_round_to_float(float value)
{
    return (float)float_round_to_int(value);
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
            const fix32_t value = fix32_from_float(inputs[index]);
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
            const fix32_t reciprocal = fix32_from_float(1.0f / inputs[index]);
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
            checksum += float_ceil_to_int(inputs[index]);
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
            checksum += float_floor_to_int(inputs[index]);
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
            checksum += float_ceil_to_float(inputs[index]);
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
            checksum += float_floor_to_float(inputs[index]);
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
            checksum += float_round_to_int(inputs[index]);
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
            checksum += float_round_to_float(inputs[index]);
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_dda_line(size_t line_length, size_t repeat_count)
{
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
    int64_t checksum = 0;
    size_t repeat;

    line_endpoints(line_length, &start_x, &start_y, &end_x, &end_y);

    {
        const int32_t dx = end_x - start_x;
        const int32_t dy = end_y - start_y;
        const int32_t steps = int_max(int_abs(dx), int_abs(dy));
        const fix32_t step_x =
            fix32_div_by_int(fix32_from_int(dx), steps);
        const fix32_t step_y =
            fix32_div_by_int(fix32_from_int(dy), steps);

        for (repeat = 0; repeat < repeat_count; ++repeat) {
            fix32_t x = fix32_from_int(start_x);
            fix32_t y = fix32_from_int(start_y);
            size_t index;

            for (index = 0; index <= (size_t)steps; ++index) {
                const int32_t pixel_x = fix32_round_to_int(x);
                const int32_t pixel_y = fix32_round_to_int(y);

                checksum += (int64_t)pixel_y * DDA_CANVAS_STRIDE + pixel_x;
                x = fix32_add(x, step_x);
                y = fix32_add(y, step_y);
            }
        }
    }

    g_int_sink = checksum;
}

static void bench_float_dda_line(size_t line_length, size_t repeat_count)
{
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
    double checksum = 0.0;
    size_t repeat;

    line_endpoints(line_length, &start_x, &start_y, &end_x, &end_y);

    {
        const int32_t dx = end_x - start_x;
        const int32_t dy = end_y - start_y;
        const int32_t steps = int_max(int_abs(dx), int_abs(dy));
        const float step_x = (float)dx / (float)steps;
        const float step_y = (float)dy / (float)steps;

        for (repeat = 0; repeat < repeat_count; ++repeat) {
            float x = (float)start_x;
            float y = (float)start_y;
            size_t index;

            for (index = 0; index <= (size_t)steps; ++index) {
                const int32_t pixel_x = float_round_to_int(x);
                const int32_t pixel_y = float_round_to_int(y);

                checksum += (double)((pixel_y * DDA_CANVAS_STRIDE) + pixel_x);
                x += step_x;
                y += step_y;
            }
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_sprite_scaler(const sprite_case_t *sprite_case,
                                      size_t repeat_count)
{
    const fix32_t step_x =
        fix32_div_by_int(fix32_from_int(sprite_case->src_width),
                           sprite_case->dest_width);
    const fix32_t step_y =
        fix32_div_by_int(fix32_from_int(sprite_case->src_height),
                           sprite_case->dest_height);
    int64_t checksum = 0;
    size_t repeat;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        fix32_t source_y = 0;
        int32_t dest_y;

        for (dest_y = 0; dest_y < sprite_case->dest_height; ++dest_y) {
            const int32_t sample_y = fix32_floor_to_int(source_y);
            fix32_t source_x = 0;
            int32_t dest_x;

            for (dest_x = 0; dest_x < sprite_case->dest_width; ++dest_x) {
                const int32_t sample_x = fix32_floor_to_int(source_x);

                checksum +=
                    (int64_t)(sample_y * sprite_case->src_width) + sample_x;
                source_x = fix32_add(source_x, step_x);
            }

            source_y = fix32_add(source_y, step_y);
        }
    }

    g_int_sink = checksum;
}

static void bench_float_sprite_scaler(const sprite_case_t *sprite_case,
                                      size_t repeat_count)
{
    const float step_x =
        (float)sprite_case->src_width / (float)sprite_case->dest_width;
    const float step_y =
        (float)sprite_case->src_height / (float)sprite_case->dest_height;
    double checksum = 0.0;
    size_t repeat;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        float source_y = 0.0f;
        int32_t dest_y;

        for (dest_y = 0; dest_y < sprite_case->dest_height; ++dest_y) {
            const int32_t sample_y = float_floor_to_int(source_y);
            float source_x = 0.0f;
            int32_t dest_x;

            for (dest_x = 0; dest_x < sprite_case->dest_width; ++dest_x) {
                const int32_t sample_x = float_floor_to_int(source_x);

                checksum +=
                    (double)((sample_y * sprite_case->src_width) + sample_x);
                source_x += step_x;
            }

            source_y += step_y;
        }
    }

    g_float_sink = checksum;
}

static void bench_fixed_rotoscaler(const rotoscale_case_t *rotoscale_case,
                                   size_t repeat_count)
{
    const fix32_t cos_angle = fix32_from_float(rotoscale_case->cos_angle);
    const fix32_t sin_angle = fix32_from_float(rotoscale_case->sin_angle);
    const fix32_t scale_x =
        fix32_div_by_int(fix32_from_int(rotoscale_case->src_width),
                           rotoscale_case->dest_width);
    const fix32_t scale_y =
        fix32_div_by_int(fix32_from_int(rotoscale_case->src_height),
                           rotoscale_case->dest_height);
    const fix32_t step_u_x = fix32_mul(cos_angle, scale_x);
    const fix32_t step_v_x = -fix32_mul(sin_angle, scale_x);
    const fix32_t step_u_y = fix32_mul(sin_angle, scale_y);
    const fix32_t step_v_y = fix32_mul(cos_angle, scale_y);
    const fix32_t half_dest_x =
        fix32_div_by_int(fix32_from_int(rotoscale_case->dest_width), 2);
    const fix32_t half_dest_y =
        fix32_div_by_int(fix32_from_int(rotoscale_case->dest_height), 2);
    const fix32_t src_center_x =
        fix32_div_by_int(fix32_from_int(rotoscale_case->src_width), 2);
    const fix32_t src_center_y =
        fix32_div_by_int(fix32_from_int(rotoscale_case->src_height), 2);
    const fix32_t origin_u =
        src_center_x - fix32_mul(half_dest_x, step_u_x) -
        fix32_mul(half_dest_y, step_u_y);
    const fix32_t origin_v =
        src_center_y - fix32_mul(half_dest_x, step_v_x) -
        fix32_mul(half_dest_y, step_v_y);
    int64_t checksum = 0;
    size_t repeat;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        fix32_t row_u = origin_u;
        fix32_t row_v = origin_v;
        int32_t dest_y;

        for (dest_y = 0; dest_y < rotoscale_case->dest_height; ++dest_y) {
            fix32_t u = row_u;
            fix32_t v = row_v;
            int32_t dest_x;

            for (dest_x = 0; dest_x < rotoscale_case->dest_width; ++dest_x) {
                const int32_t sample_x = fix32_floor_to_int(u);
                const int32_t sample_y = fix32_floor_to_int(v);

                checksum +=
                    (int64_t)sample_y * rotoscale_case->src_width + sample_x;
                u = fix32_add(u, step_u_x);
                v = fix32_add(v, step_v_x);
            }

            row_u = fix32_add(row_u, step_u_y);
            row_v = fix32_add(row_v, step_v_y);
        }
    }

    g_int_sink = checksum;
}

static void bench_float_rotoscaler(const rotoscale_case_t *rotoscale_case,
                                   size_t repeat_count)
{
    const float scale_x =
        (float)rotoscale_case->src_width / (float)rotoscale_case->dest_width;
    const float scale_y =
        (float)rotoscale_case->src_height / (float)rotoscale_case->dest_height;
    const float step_u_x = rotoscale_case->cos_angle * scale_x;
    const float step_v_x = -rotoscale_case->sin_angle * scale_x;
    const float step_u_y = rotoscale_case->sin_angle * scale_y;
    const float step_v_y = rotoscale_case->cos_angle * scale_y;
    const float half_dest_x = (float)rotoscale_case->dest_width * 0.5f;
    const float half_dest_y = (float)rotoscale_case->dest_height * 0.5f;
    const float src_center_x = (float)rotoscale_case->src_width * 0.5f;
    const float src_center_y = (float)rotoscale_case->src_height * 0.5f;
    const float origin_u =
        src_center_x - (half_dest_x * step_u_x) - (half_dest_y * step_u_y);
    const float origin_v =
        src_center_y - (half_dest_x * step_v_x) - (half_dest_y * step_v_y);
    double checksum = 0.0;
    size_t repeat;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        float row_u = origin_u;
        float row_v = origin_v;
        int32_t dest_y;

        for (dest_y = 0; dest_y < rotoscale_case->dest_height; ++dest_y) {
            float u = row_u;
            float v = row_v;
            int32_t dest_x;

            for (dest_x = 0; dest_x < rotoscale_case->dest_width; ++dest_x) {
                const int32_t sample_x = float_floor_to_int(u);
                const int32_t sample_y = float_floor_to_int(v);

                checksum +=
                    (double)((sample_y * rotoscale_case->src_width) + sample_x);
                u += step_u_x;
                v += step_v_x;
            }

            row_u += step_u_y;
            row_v += step_v_y;
        }
    }

    g_float_sink = checksum;
}

static void print_scalar_results(const benchmark_data_t *data)
{
    static const benchmark_pair_t pairs[] = {
        { "int -> representation", bench_fixed_from_int, bench_float_from_int },
        { "float -> representation", bench_fixed_from_float, bench_float_from_float },
        { "sum", bench_fixed_sum, bench_float_sum },
        { "multiply", bench_fixed_mul, bench_float_mul },
        { "reciprocal -> fixed", bench_fixed_reciprocal, bench_float_reciprocal },
        { "divide", bench_fixed_div, bench_float_div },
        { "mul reciprocal", bench_fixed_mul_reciprocal, bench_float_mul_reciprocal },
        { "ceil -> int", bench_fixed_ceil_to_int, bench_float_ceil_to_int },
        { "floor -> int", bench_fixed_floor_to_int, bench_float_floor_to_int },
        { "round -> int", bench_fixed_round_to_int, bench_float_round_to_int },
        { "value -> float", bench_fixed_to_float, bench_float_to_float },
        { "ceil -> float", bench_fixed_ceil_to_float, bench_float_ceil_to_float },
        { "floor -> float", bench_fixed_floor_to_float, bench_float_floor_to_float },
        { "round -> float", bench_fixed_round_to_float, bench_float_round_to_float },
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

static void print_line_results(size_t repeat_count)
{
    static const size_t line_lengths[] = { 32u, 128u, 512u, 2048u, 8192u };
    const size_t case_count = sizeof(line_lengths) / sizeof(line_lengths[0]);
    size_t index;

    printf("\nDDA line benchmarks\n");
    printf("repeats-per-case=%lu\n", (unsigned long)repeat_count);
    printf("%-12s %12s %14s %14s %12s\n",
           "line length", "pixels", "fixed ns/pixel", "float ns/pixel",
           "fixed/float");

    for (index = 0; index < case_count; ++index) {
        const size_t line_length = line_lengths[index];
        const size_t point_count = line_point_count(line_length);
        const double operations =
            (double)point_count * (double)repeat_count;
        const double fixed_seconds =
            run_line_benchmark(bench_fixed_dda_line, line_length, repeat_count);
        const double float_seconds =
            run_line_benchmark(bench_float_dda_line, line_length, repeat_count);
        const double fixed_ns = (fixed_seconds * 1.0e9) / operations;
        const double float_ns = (float_seconds * 1.0e9) / operations;
        const double ratio = (float_ns > 0.0) ? (fixed_ns / float_ns) : 0.0;

        printf("%-12lu %12lu %14.2f %14.2f %12.2f\n",
               (unsigned long)line_length,
               (unsigned long)point_count,
               fixed_ns, float_ns, ratio);
    }
}

static void print_sprite_results(size_t repeat_count)
{
    static const sprite_case_t sprite_cases[] = {
        { "16x16 -> 24x24 (1.5x)", 16, 16, 24, 24 },
        { "32x24 -> 64x42 (2.0x/1.75x)", 32, 24, 64, 42 },
        { "64x48 -> 160x96 (2.5x/2.0x)", 64, 48, 160, 96 },
        { "96x64 -> 288x160 (3.0x/2.5x)", 96, 64, 288, 160 },
        { "128x96 -> 512x384 (4.0x)", 128, 96, 512, 384 },
        { "160x128 -> 1024x768 (6.4x)", 160, 128, 1024, 768 },
        { "256x192 -> 2048x1536 (8.0x)", 256, 192, 2048, 1536 },
        { "512x384 -> 4096x3072 (8.0x)", 512, 384, 4096, 3072 },
        { "1024x768 -> 8192x6144 (8.0x)", 1024, 768, 8192, 6144 },
    };
    const size_t case_count = sizeof(sprite_cases) / sizeof(sprite_cases[0]);
    size_t index;

    printf("\nsprite scaler benchmarks\n");
    printf("repeats-per-case=%lu\n", (unsigned long)repeat_count);
    printf("%-32s %12s %14s %14s %12s\n",
           "case", "dest pixels", "fixed ns/pixel", "float ns/pixel",
           "fixed/float");

    for (index = 0; index < case_count; ++index) {
        const sprite_case_t *sprite_case = &sprite_cases[index];
        const double operations =
            (double)(sprite_case->dest_width * sprite_case->dest_height) *
            (double)repeat_count;
        const double fixed_seconds =
            run_sprite_benchmark(bench_fixed_sprite_scaler, sprite_case,
                                 repeat_count);
        const double float_seconds =
            run_sprite_benchmark(bench_float_sprite_scaler, sprite_case,
                                 repeat_count);
        const double fixed_ns = (fixed_seconds * 1.0e9) / operations;
        const double float_ns = (float_seconds * 1.0e9) / operations;
        const double ratio = (float_ns > 0.0) ? (fixed_ns / float_ns) : 0.0;

        printf("%-32s %12lu %14.2f %14.2f %12.2f\n",
               sprite_case->label,
               (unsigned long)(sprite_case->dest_width * sprite_case->dest_height),
               fixed_ns, float_ns, ratio);
    }
}

static void print_rotoscale_results(size_t repeat_count)
{
    static const rotoscale_case_t rotoscale_cases[] = {
        { "32x32 -> 64x64 @ 15 deg", 32, 32, 64, 64, 0.9659258f, 0.2588190f },
        { "64x48 -> 160x120 @ 30 deg", 64, 48, 160, 120, 0.8660254f, 0.5000000f },
        { "96x64 -> 288x192 @ 45 deg", 96, 64, 288, 192, 0.7071068f, 0.7071068f },
        { "128x96 -> 512x384 @ 22.5 deg", 128, 96, 512, 384, 0.9238795f, 0.3826834f },
        { "256x192 -> 1024x768 @ 60 deg", 256, 192, 1024, 768, 0.5000000f, 0.8660254f },
    };
    const size_t case_count = sizeof(rotoscale_cases) / sizeof(rotoscale_cases[0]);
    size_t index;

    printf("\nrotoscaler benchmarks\n");
    printf("repeats-per-case=%lu\n", (unsigned long)repeat_count);
    printf("%-36s %12s %14s %14s %12s\n",
           "case", "dest pixels", "fixed ns/pixel", "float ns/pixel",
           "fixed/float");

    for (index = 0; index < case_count; ++index) {
        const rotoscale_case_t *rotoscale_case = &rotoscale_cases[index];
        const double operations =
            (double)(rotoscale_case->dest_width * rotoscale_case->dest_height) *
            (double)repeat_count;
        const double fixed_seconds =
            run_rotoscale_benchmark(bench_fixed_rotoscaler, rotoscale_case,
                                    repeat_count);
        const double float_seconds =
            run_rotoscale_benchmark(bench_float_rotoscaler, rotoscale_case,
                                    repeat_count);
        const double fixed_ns = (fixed_seconds * 1.0e9) / operations;
        const double float_ns = (float_seconds * 1.0e9) / operations;
        const double ratio = (float_ns > 0.0) ? (fixed_ns / float_ns) : 0.0;

        printf("%-36s %12lu %14.2f %14.2f %12.2f\n",
               rotoscale_case->label,
               (unsigned long)(rotoscale_case->dest_width *
                               rotoscale_case->dest_height),
               fixed_ns, float_ns, ratio);
    }
}

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "usage: %s [sample-count] [repeat-count]\n"
            "defaults: sample-count=%u repeat-count=%u\n",
            program_name, DEFAULT_SAMPLE_COUNT, DEFAULT_REPEAT_COUNT);
}

int main(int argc, char **argv)
{
    benchmark_data_t data;
    size_t sample_count = DEFAULT_SAMPLE_COUNT;
    size_t repeat_count = DEFAULT_REPEAT_COUNT;

    if (argc > 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 2 && !parse_size_arg(argv[1], &sample_count)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3 && !parse_size_arg(argv[2], &repeat_count)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!benchmark_data_init(&data, sample_count, repeat_count)) {
        benchmark_data_destroy(&data);
        return EXIT_FAILURE;
    }

    print_scalar_results(&data);
    print_line_results(repeat_count);
    print_sprite_results(repeat_count);
    print_rotoscale_results(repeat_count);
    benchmark_data_destroy(&data);
    return EXIT_SUCCESS;
}
