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

typedef struct sprite_case {
    const char *label;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_width;
    int32_t dest_height;
} sprite_case_t;

typedef void (*sprite_benchmark_fn)(const sprite_case_t *sprite_case,
                                    size_t repeat_count);

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
    int64_t checksum = 0;
    size_t repeat;

    for (repeat = 0; repeat < repeat_count; ++repeat) {
        float source_y = 0.0f;
        int32_t dest_y;

        for (dest_y = 0; dest_y < sprite_case->dest_height; ++dest_y) {
            const int32_t sample_y =
                benchmark_float_floor_to_int(source_y);
            float source_x = 0.0f;
            int32_t dest_x;

            for (dest_x = 0; dest_x < sprite_case->dest_width; ++dest_x) {
                const int32_t sample_x =
                    benchmark_float_floor_to_int(source_x);

                checksum +=
                    (int64_t)sample_y * sprite_case->src_width + sample_x;
                source_x += step_x;
            }

            source_y += step_y;
        }
    }

    g_int_sink = checksum;
}

void benchmark_print_sprite_results(size_t repeat_count)
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
               (unsigned long)(sprite_case->dest_width *
                               sprite_case->dest_height),
               fixed_ns, float_ns, ratio);
    }
}
