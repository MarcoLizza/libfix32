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

#ifndef FIX32_TEST_H
#define FIX32_TEST_H

#include <stdint.h>

#include "fix32.h"

#define TEST_EXPECT_EQ(expected, actual) \
    test_expect_int64((int64_t)(expected), (int64_t)(actual), #actual, \
                      __FILE__, __LINE__)

#define TEST_EXPECT_FLOAT_NEAR(expected, actual, tolerance) \
    test_expect_float_near((expected), (actual), (tolerance), #actual, \
                           __FILE__, __LINE__)

#define TEST_EXPECT_DOUBLE_NEAR(expected, actual, tolerance) \
    test_expect_double_near((expected), (actual), (tolerance), #actual, \
                            __FILE__, __LINE__)

extern unsigned int g_test_assertions;
extern unsigned int g_test_failures;

void test_expect_int64(int64_t expected, int64_t actual,
                       const char *expression, const char *file, int line);
void test_expect_float_near(float expected, float actual, float tolerance,
                            const char *expression, const char *file, int line);
void test_expect_double_near(double expected, double actual, double tolerance,
                             const char *expression, const char *file, int line);

void test_conversions(void);
void test_arithmetic(void);
void test_rounding(void);

#endif
