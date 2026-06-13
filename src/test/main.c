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

#include "test.h"

#include <stdio.h>
#include <stdlib.h>

unsigned int g_test_assertions = 0;
unsigned int g_test_failures = 0;

void test_expect_int64(int64_t expected, int64_t actual,
                       const char *expression, const char *file, int line)
{
    ++g_test_assertions;

    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %lld, got %lld\n",
                file, line, expression, (long long)expected, (long long)actual);
        ++g_test_failures;
    }
}

void test_expect_float_near(float expected, float actual, float tolerance,
                            const char *expression, const char *file, int line)
{
    const float difference = (actual >= expected)
        ? actual - expected
        : expected - actual;

    ++g_test_assertions;

    if (difference > tolerance) {
        fprintf(stderr, "%s:%d: %s: expected %.9g +/- %.9g, got %.9g\n",
                file, line, expression, (double)expected, (double)tolerance,
                (double)actual);
        ++g_test_failures;
    }
}

void test_expect_double_near(double expected, double actual, double tolerance,
                             const char *expression, const char *file, int line)
{
    const double difference = (actual >= expected)
        ? actual - expected
        : expected - actual;

    ++g_test_assertions;

    if (difference > tolerance) {
        fprintf(stderr, "%s:%d: %s: expected %.17g +/- %.17g, got %.17g\n",
                file, line, expression, expected, tolerance, actual);
        ++g_test_failures;
    }
}

int main(void)
{
    test_conversions();
    test_arithmetic();
    test_rounding();

    if (g_test_failures != 0u) {
        fprintf(stderr, "%u of %u assertions failed\n",
                g_test_failures, g_test_assertions);
        return EXIT_FAILURE;
    }

    printf("all %u assertions passed\n", g_test_assertions);
    return EXIT_SUCCESS;
}
