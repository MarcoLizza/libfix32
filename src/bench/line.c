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

#define DDA_CANVAS_STRIDE 4099

typedef void (*line_benchmark_fn)(size_t line_length, size_t repeat_count);

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
    return (size_t)(benchmark_int_max(benchmark_int_abs(end_x - start_x),
                                     benchmark_int_abs(end_y - start_y)) + 1);
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
        const int32_t steps =
            benchmark_int_max(benchmark_int_abs(dx), benchmark_int_abs(dy));
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
    int64_t checksum = 0;
    size_t repeat;

    line_endpoints(line_length, &start_x, &start_y, &end_x, &end_y);

    {
        const int32_t dx = end_x - start_x;
        const int32_t dy = end_y - start_y;
        const int32_t steps =
            benchmark_int_max(benchmark_int_abs(dx), benchmark_int_abs(dy));
        const float step_x = (float)dx / (float)steps;
        const float step_y = (float)dy / (float)steps;

        for (repeat = 0; repeat < repeat_count; ++repeat) {
            float x = (float)start_x;
            float y = (float)start_y;
            size_t index;

            for (index = 0; index <= (size_t)steps; ++index) {
                const int32_t pixel_x = benchmark_float_round_to_int(x);
                const int32_t pixel_y = benchmark_float_round_to_int(y);

                checksum +=
                    (int64_t)pixel_y * DDA_CANVAS_STRIDE + pixel_x;
                x += step_x;
                y += step_y;
            }
        }
    }

    g_int_sink = checksum;
}

void benchmark_print_line_results(size_t repeat_count)
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
               (unsigned long)line_length, (unsigned long)point_count,
               fixed_ns, float_ns, ratio);
    }
}
