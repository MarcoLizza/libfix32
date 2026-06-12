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

#include <limits.h>

void test_conversions(void)
{
    TEST_EXPECT_EQ(INT32_MIN, fix32_raw(fix32_from_raw(INT32_MIN)));
    TEST_EXPECT_EQ(-1, fix32_raw(fix32_from_raw(-1)));
    TEST_EXPECT_EQ(0, fix32_raw(fix32_from_raw(0)));
    TEST_EXPECT_EQ(INT32_MAX, fix32_raw(fix32_from_raw(INT32_MAX)));

    TEST_EXPECT_EQ(INT32_MIN, fix32_from_int(FIX32_MIN_INT_INPUT));
    TEST_EXPECT_EQ(-FIX32_SCALE, fix32_from_int(-1));
    TEST_EXPECT_EQ(0, fix32_from_int(0));
    TEST_EXPECT_EQ(FIX32_SCALE, fix32_from_int(1));
    TEST_EXPECT_EQ((int64_t)FIX32_MAX_INT_INPUT * FIX32_SCALE,
                   fix32_from_int(FIX32_MAX_INT_INPUT));

    TEST_EXPECT_EQ(0, fix32_from_float(0.0f));
    TEST_EXPECT_EQ(FIX32_SCALE / 4, fix32_from_float(0.25f));
    TEST_EXPECT_EQ(FIX32_SCALE + FIX32_HALF, fix32_from_float(1.5f));
    TEST_EXPECT_EQ(-(FIX32_SCALE + FIX32_HALF), fix32_from_float(-1.5f));
    TEST_EXPECT_EQ(1, fix32_from_float(1.0f / (float)FIX32_SCALE));
    TEST_EXPECT_EQ(-1, fix32_from_float(-1.0f / (float)FIX32_SCALE));

    TEST_EXPECT_FLOAT_NEAR(0.0f, fix32_to_float(0), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(0.25f,
                           fix32_to_float(fix32_from_raw(FIX32_SCALE / 4)),
                           0.0f);
    TEST_EXPECT_FLOAT_NEAR(-1.5f,
                           fix32_to_float(
                               fix32_from_raw(-(FIX32_SCALE + FIX32_HALF))),
                           0.0f);
}
