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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int64_t g_int_sink = 0;
volatile double g_float_sink = 0.0;

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

int benchmark_data_init(benchmark_data_t *data, size_t sample_count,
                        size_t repeat_count)
{
    uint32_t state = 0x9e3779b9u;
    size_t index;

    memset(data, 0, sizeof(*data));
    data->sample_count = sample_count;
    data->repeat_count = repeat_count;

    data->int_inputs = checked_alloc(sample_count, sizeof(*data->int_inputs));
    data->small_int_inputs =
        checked_alloc(sample_count, sizeof(*data->small_int_inputs));
    data->int_div_inputs =
        checked_alloc(sample_count, sizeof(*data->int_div_inputs));
    data->rational_numerators =
        checked_alloc(sample_count, sizeof(*data->rational_numerators));
    data->rational_denominators =
        checked_alloc(sample_count, sizeof(*data->rational_denominators));
    data->float_inputs = checked_alloc(sample_count, sizeof(*data->float_inputs));
    data->double_inputs =
        checked_alloc(sample_count, sizeof(*data->double_inputs));
    data->sum_inputs = checked_alloc(sample_count, sizeof(*data->sum_inputs));
    data->lerp_inputs = checked_alloc(sample_count, sizeof(*data->lerp_inputs));
    data->div_inputs = checked_alloc(sample_count, sizeof(*data->div_inputs));
    data->float_reciprocals =
        checked_alloc(sample_count, sizeof(*data->float_reciprocals));
    data->fixed_inputs = checked_alloc(sample_count, sizeof(*data->fixed_inputs));
    data->sum_fixed_inputs =
        checked_alloc(sample_count, sizeof(*data->sum_fixed_inputs));
    data->fixed_lerp_inputs =
        checked_alloc(sample_count, sizeof(*data->fixed_lerp_inputs));
    data->fixed_div_inputs =
        checked_alloc(sample_count, sizeof(*data->fixed_div_inputs));
    data->fixed_reciprocals =
        checked_alloc(sample_count, sizeof(*data->fixed_reciprocals));

    if (data->int_inputs == NULL || data->small_int_inputs == NULL ||
        data->int_div_inputs == NULL || data->rational_numerators == NULL ||
        data->rational_denominators == NULL || data->float_inputs == NULL ||
        data->double_inputs == NULL || data->sum_inputs == NULL ||
        data->lerp_inputs == NULL || data->div_inputs == NULL ||
        data->float_reciprocals == NULL || data->fixed_inputs == NULL ||
        data->sum_fixed_inputs == NULL || data->fixed_lerp_inputs == NULL ||
        data->fixed_div_inputs == NULL || data->fixed_reciprocals == NULL) {
        return 0;
    }

    for (index = 0; index < sample_count; ++index) {
        data->int_inputs[index] = (int32_t)(lcg_next(&state) % 60001u) - 30000;
        data->small_int_inputs[index] =
            (int32_t)(lcg_next(&state) % 17u) - 8;
        data->int_div_inputs[index] =
            (int32_t)(lcg_next(&state) % 16u) + 1;
        if ((lcg_next(&state) & 1u) != 0u) {
            data->int_div_inputs[index] = -data->int_div_inputs[index];
        }
        data->rational_numerators[index] =
            (int32_t)(lcg_next(&state) % 4097u) - 2048;
        data->rational_denominators[index] =
            (int32_t)(lcg_next(&state) % 64u) + 1;
        if ((lcg_next(&state) & 1u) != 0u) {
            data->rational_denominators[index] =
                -data->rational_denominators[index];
        }
        data->float_inputs[index] = random_float(&state, -256.0f, 256.0f);
        data->double_inputs[index] = (double)data->float_inputs[index];
        data->sum_inputs[index] = random_float(&state, -0.5f, 0.5f);
        data->div_inputs[index] = random_divisor(&state);
        data->float_reciprocals[index] = 1.0f / data->div_inputs[index];
        data->fixed_inputs[index] =
            fix32_round_from_float(data->float_inputs[index]);
        data->sum_fixed_inputs[index] =
            fix32_round_from_float(data->sum_inputs[index]);
        data->fixed_div_inputs[index] =
            fix32_round_from_float(data->div_inputs[index]);
        data->fixed_reciprocals[index] =
            fix32_reciprocal(data->fixed_div_inputs[index]);
    }

    for (index = 0; index < sample_count; ++index) {
        data->lerp_inputs[index] = random_float(&state, 0.0f, 1.0f);
        data->fixed_lerp_inputs[index] =
            fix32_round_from_float(data->lerp_inputs[index]);
    }

    return 1;
}

void benchmark_data_destroy(benchmark_data_t *data)
{
    free(data->int_inputs);
    free(data->small_int_inputs);
    free(data->int_div_inputs);
    free(data->rational_numerators);
    free(data->rational_denominators);
    free(data->float_inputs);
    free(data->double_inputs);
    free(data->sum_inputs);
    free(data->lerp_inputs);
    free(data->div_inputs);
    free(data->float_reciprocals);
    free(data->fixed_inputs);
    free(data->sum_fixed_inputs);
    free(data->fixed_lerp_inputs);
    free(data->fixed_div_inputs);
    free(data->fixed_reciprocals);
}

int benchmark_parse_size_arg(const char *text, size_t *value)
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
