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

static void bench_fixed_rotoscaler(const rotoscale_case_t *rotoscale_case,
                                   size_t repeat_count)
{
    const fix32_t cos_angle =
        fix32_round_from_float(rotoscale_case->cos_angle);
    const fix32_t sin_angle =
        fix32_round_from_float(rotoscale_case->sin_angle);
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
    int64_t checksum = 0;
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
                const int32_t sample_x =
                    benchmark_float_floor_to_int(u);
                const int32_t sample_y =
                    benchmark_float_floor_to_int(v);

                checksum +=
                    (int64_t)sample_y * rotoscale_case->src_width + sample_x;
                u += step_u_x;
                v += step_v_x;
            }

            row_u += step_u_y;
            row_v += step_v_y;
        }
    }

    g_int_sink = checksum;
}

void benchmark_print_rotoscale_results(size_t repeat_count)
{
    static const rotoscale_case_t rotoscale_cases[] = {
        { "32x32 -> 64x64 @ 15 deg",
          32, 32, 64, 64, 0.9659258f, 0.2588190f },
        { "64x48 -> 160x120 @ 30 deg",
          64, 48, 160, 120, 0.8660254f, 0.5000000f },
        { "96x64 -> 288x192 @ 45 deg",
          96, 64, 288, 192, 0.7071068f, 0.7071068f },
        { "128x96 -> 512x384 @ 22.5 deg",
          128, 96, 512, 384, 0.9238795f, 0.3826834f },
        { "256x192 -> 1024x768 @ 60 deg",
          256, 192, 1024, 768, 0.5000000f, 0.8660254f },
    };
    const size_t case_count =
        sizeof(rotoscale_cases) / sizeof(rotoscale_cases[0]);
    size_t index;

    printf("\nrotoscaler benchmarks\n");
    printf("repeats-per-case=%lu\n", (unsigned long)repeat_count);
    printf("%-36s %12s %14s %14s %12s\n",
           "case", "dest pixels", "fixed ns/pixel", "float ns/pixel",
           "fixed/float");

    for (index = 0; index < case_count; ++index) {
        const rotoscale_case_t *rotoscale_case = &rotoscale_cases[index];
        const double operations =
            (double)(rotoscale_case->dest_width *
                     rotoscale_case->dest_height) *
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
