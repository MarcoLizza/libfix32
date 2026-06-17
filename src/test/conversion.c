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
    const float positive_float = 1.75f / (float)FIX32_ONE;
    const float negative_float = -1.75f / (float)FIX32_ONE;
    const double positive_double = 1.75 / (double)FIX32_ONE;
    const double negative_double = -1.75 / (double)FIX32_ONE;

    TEST_EXPECT_EQ(FIX32_ONE / 2, FIX32_HALF);
    TEST_EXPECT_EQ(FIX32_ONE - 1, FIX32_FRACTIONAL_MASK);
    TEST_EXPECT_EQ(~FIX32_FRACTIONAL_MASK, FIX32_INTEGER_MASK);
    TEST_EXPECT_EQ(INT32_MIN, (int64_t)FIX32_INT_MIN * FIX32_ONE);

    TEST_EXPECT_EQ(INT32_MIN, fix32_to_raw(fix32_from_raw(INT32_MIN)));
    TEST_EXPECT_EQ(-1, fix32_to_raw(fix32_from_raw(-1)));
    TEST_EXPECT_EQ(0, fix32_to_raw(fix32_from_raw(0)));
    TEST_EXPECT_EQ(INT32_MAX, fix32_to_raw(fix32_from_raw(INT32_MAX)));

    TEST_EXPECT_EQ(INT32_MIN, fix32_from_int(FIX32_INT_MIN));
    TEST_EXPECT_EQ(-FIX32_ONE, fix32_from_int(-1));
    TEST_EXPECT_EQ(0, fix32_from_int(0));
    TEST_EXPECT_EQ(FIX32_ONE, fix32_from_int(1));
    TEST_EXPECT_EQ((int64_t)FIX32_INT_MAX * FIX32_ONE,
                   fix32_from_int(FIX32_INT_MAX));

    TEST_EXPECT_EQ(0, fix32_from_float(0.0f));
    TEST_EXPECT_EQ(FIX32_ONE / 4, fix32_from_float(0.25f));
    TEST_EXPECT_EQ(FIX32_ONE + FIX32_HALF, fix32_from_float(1.5f));
    TEST_EXPECT_EQ(-(FIX32_ONE + FIX32_HALF), fix32_from_float(-1.5f));
    TEST_EXPECT_EQ(1, fix32_from_float(positive_float));
    TEST_EXPECT_EQ(-1, fix32_from_float(negative_float));
    TEST_EXPECT_EQ(2, fix32_round_from_float(positive_float));
    TEST_EXPECT_EQ(-2, fix32_round_from_float(negative_float));

    TEST_EXPECT_EQ(0, fix32_from_double(0.0));
    TEST_EXPECT_EQ(FIX32_ONE / 4, fix32_from_double(0.25));
    TEST_EXPECT_EQ(FIX32_ONE + FIX32_HALF, fix32_from_double(1.5));
    TEST_EXPECT_EQ(-(FIX32_ONE + FIX32_HALF), fix32_from_double(-1.5));
    TEST_EXPECT_EQ(1, fix32_from_double(positive_double));
    TEST_EXPECT_EQ(-1, fix32_from_double(negative_double));
    TEST_EXPECT_EQ(2, fix32_round_from_double(positive_double));
    TEST_EXPECT_EQ(-2, fix32_round_from_double(negative_double));

    TEST_EXPECT_EQ(0, fix32_from_rational(0, 7));
    TEST_EXPECT_EQ(FIX32_ONE / 2, fix32_from_rational(1, 2));
    TEST_EXPECT_EQ(-(FIX32_ONE / 2), fix32_from_rational(-1, 2));
    TEST_EXPECT_EQ(-(FIX32_ONE / 2), fix32_from_rational(1, -2));
    TEST_EXPECT_EQ(FIX32_ONE / 2, fix32_from_rational(-1, -2));
    TEST_EXPECT_EQ((7 * FIX32_ONE) / 3, fix32_from_rational(7, 3));
    TEST_EXPECT_EQ((-7 * FIX32_ONE) / 3, fix32_from_rational(-7, 3));
    TEST_EXPECT_EQ((355 * FIX32_ONE) / 113,
                   fix32_from_rational(355, 113));

    TEST_EXPECT_FLOAT_NEAR(0.0f, fix32_to_float(0), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(0.25f,
                           fix32_to_float(fix32_from_raw(FIX32_ONE / 4)),
                           0.0f);
    TEST_EXPECT_FLOAT_NEAR(-1.5f,
                           fix32_to_float(
                               fix32_from_raw(-(FIX32_ONE + FIX32_HALF))),
                           0.0f);

    TEST_EXPECT_DOUBLE_NEAR(0.0, fix32_to_double(0), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(
        0.25, fix32_to_double(fix32_from_raw(FIX32_ONE / 4)), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(
        -1.5,
        fix32_to_double(fix32_from_raw(-(FIX32_ONE + FIX32_HALF))), 0.0);

    TEST_EXPECT_EQ(fix32_from_int(2), FIX32_FROM_INT(2));
    TEST_EXPECT_EQ(fix32_from_int(-2), FIX32_FROM_INT(-2));
    TEST_EXPECT_EQ(fix32_from_rational(7, 3), FIX32_FROM_RATIONAL(7, 3));
#if !defined(FIX32_NO_ROUNDING)
    TEST_EXPECT_EQ(2, FIX32_FROM_FLOAT(positive_float));
    TEST_EXPECT_EQ(2, FIX32_FROM_DOUBLE(positive_double));
    TEST_EXPECT_EQ(2, FIX32_TO_INT(fix32_from_float(1.75f)));
    TEST_EXPECT_EQ(-2, FIX32_FROM_FLOAT(negative_float));
    TEST_EXPECT_EQ(-2, FIX32_FROM_DOUBLE(negative_double));
    TEST_EXPECT_EQ(-2, FIX32_TO_INT(fix32_from_float(-1.75f)));
#else
    TEST_EXPECT_EQ(1, FIX32_FROM_FLOAT(positive_float));
    TEST_EXPECT_EQ(1, FIX32_FROM_DOUBLE(positive_double));
    TEST_EXPECT_EQ(1, FIX32_TO_INT(fix32_from_float(1.75f)));
    TEST_EXPECT_EQ(-1, FIX32_FROM_FLOAT(negative_float));
    TEST_EXPECT_EQ(-1, FIX32_FROM_DOUBLE(negative_double));
    TEST_EXPECT_EQ(-1, FIX32_TO_INT(fix32_from_float(-1.75f)));
#endif
    TEST_EXPECT_FLOAT_NEAR(1.5f,
                           FIX32_TO_FLOAT(fix32_from_float(1.5f)), 0.0f);
    TEST_EXPECT_DOUBLE_NEAR(1.5,
                            FIX32_TO_DOUBLE(fix32_from_double(1.5)), 0.0);
#ifdef TEST_EXPECT_USE_64_BIT
    TEST_EXPECT_EQ(TEST_EXPECT_USE_64_BIT, FIX32_USE_64_BIT);
#endif
#ifdef TEST_EXPECT_SIGNED_SHIFT_MUL
    TEST_EXPECT_EQ(TEST_EXPECT_SIGNED_SHIFT_MUL,
                   FIX32_USE_SIGNED_SHIFT_MUL);
#endif
#ifdef TEST_EXPECT_SIGNED_SHIFT_DIV
    TEST_EXPECT_EQ(TEST_EXPECT_SIGNED_SHIFT_DIV,
                   FIX32_USE_SIGNED_SHIFT_DIV);
#endif
}
