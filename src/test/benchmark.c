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

#if !defined(FIX32_STATIC_INLINE)
#define FIX32_IMPLEMENTATION
#endif
#include "common.h"

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int g_call_sides[2 * (BENCHMARK_TRIAL_COUNT + 1)];
static size_t g_call_count;

static void fixed_trial(const void *context, size_t repeat_count)
{
    (void)context;
    assert(g_call_count < sizeof(g_call_sides) / sizeof(g_call_sides[0]));
    assert(repeat_count == ((g_call_count < 2) ? 1u : 3u));
    g_call_sides[g_call_count++] = 0;
}

static void float_trial(const void *context, size_t repeat_count)
{
    (void)context;
    assert(g_call_count < sizeof(g_call_sides) / sizeof(g_call_sides[0]));
    assert(repeat_count == ((g_call_count < 2) ? 1u : 3u));
    g_call_sides[g_call_count++] = 1;
}

int main(void)
{
    benchmark_data_t data;
    unsigned int integer_divisors[33] = { 0 };
    unsigned int rational_denominators[129] = { 0 };
    size_t index;

    if (!benchmark_data_init(&data, 8192, 1)) {
        benchmark_data_destroy(&data);
        return EXIT_FAILURE;
    }
    for (index = 0; index < data.sample_count; ++index) {
        const int32_t divisor = data.int_div_inputs[index];
        const int32_t denominator = data.rational_denominators[index];

        assert(divisor >= -16 && divisor <= 16 && divisor != 0);
        assert(denominator >= -64 && denominator <= 64 && denominator != 0);
        ++integer_divisors[divisor + 16];
        ++rational_denominators[denominator + 64];
    }
    for (index = 0; index < 33; ++index) {
        assert(index == 16 || integer_divisors[index] != 0);
    }
    for (index = 0; index < 129; ++index) {
        assert(index == 64 || rational_denominators[index] != 0);
    }
    benchmark_data_destroy(&data);

    benchmark_measure_pair(fixed_trial, float_trial, NULL, 3);
    assert(g_call_count == sizeof(g_call_sides) / sizeof(g_call_sides[0]));
    assert(g_call_sides[0] == 0 && g_call_sides[1] == 1);
    for (index = 0; index < BENCHMARK_TRIAL_COUNT; ++index) {
        assert(g_call_sides[2 + 2 * index] == index % 2);
        assert(g_call_sides[3 + 2 * index] == 1 - index % 2);
    }
    puts("benchmark divisor coverage and trial ordering passed");
    return EXIT_SUCCESS;
}
