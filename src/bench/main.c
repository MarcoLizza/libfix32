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
#include <stdlib.h>

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

    if (argc >= 2 && !benchmark_parse_size_arg(argv[1], &sample_count)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3 && !benchmark_parse_size_arg(argv[2], &repeat_count)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!benchmark_data_init(&data, sample_count, repeat_count)) {
        benchmark_data_destroy(&data);
        return EXIT_FAILURE;
    }

    benchmark_print_scalar_results(&data);
    benchmark_print_line_results(repeat_count);
    benchmark_print_sprite_results(repeat_count);
    benchmark_print_rotoscale_results(repeat_count);
    benchmark_data_destroy(&data);
    return EXIT_SUCCESS;
}
