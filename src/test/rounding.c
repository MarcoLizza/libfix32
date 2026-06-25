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

void test_rounding(void)
{
    const fix32_t positive = fix32_from_float(1.75f);
    const fix32_t negative = fix32_from_float(-1.75f);
    const fix32_t positive_half = fix32_from_float(1.5f);
    const fix32_t negative_half = fix32_from_float(-1.5f);
    const fix32_t below_positive_half =
        fix32_from_raw(FIX32_ONE + FIX32_HALF - 1);
    const fix32_t above_negative_half =
        fix32_from_raw(-(FIX32_ONE + FIX32_HALF - 1));

    TEST_EXPECT_EQ(1, fix32_floor_to_int(positive));
    TEST_EXPECT_EQ(-2, fix32_floor_to_int(negative));
    TEST_EXPECT_EQ(2, fix32_floor_to_int(fix32_from_int(2)));
    TEST_EXPECT_EQ(-2, fix32_floor_to_int(fix32_from_int(-2)));

    TEST_EXPECT_EQ(2, fix32_ceil_to_int(positive));
    TEST_EXPECT_EQ(-1, fix32_ceil_to_int(negative));
    TEST_EXPECT_EQ(2, fix32_ceil_to_int(fix32_from_int(2)));
    TEST_EXPECT_EQ(-2, fix32_ceil_to_int(fix32_from_int(-2)));

    TEST_EXPECT_EQ(1, fix32_trunc_to_int(positive));
    TEST_EXPECT_EQ(-1, fix32_trunc_to_int(negative));
    TEST_EXPECT_EQ(2, fix32_round_to_int(positive_half));
    TEST_EXPECT_EQ(-2, fix32_round_to_int(negative_half));
    TEST_EXPECT_EQ(2, fix32_round_to_int(fix32_from_int(2)));
    TEST_EXPECT_EQ(-2, fix32_round_to_int(fix32_from_int(-2)));
    TEST_EXPECT_EQ(1, fix32_round_to_int(below_positive_half));
    TEST_EXPECT_EQ(-1, fix32_round_to_int(above_negative_half));
    TEST_EXPECT_EQ(0, fix32_round_to_int(0));

    TEST_EXPECT_FLOAT_NEAR(1.0f, fix32_floor_to_float(positive), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(-2.0f, fix32_floor_to_float(negative), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(2.0f, fix32_ceil_to_float(positive), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(-1.0f, fix32_ceil_to_float(negative), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(2.0f, fix32_round_to_float(positive_half), 0.0f);
    TEST_EXPECT_FLOAT_NEAR(-2.0f, fix32_round_to_float(negative_half), 0.0f);

    TEST_EXPECT_DOUBLE_NEAR(1.0, fix32_floor_to_double(positive), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(-2.0, fix32_floor_to_double(negative), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(2.0, fix32_ceil_to_double(positive), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(-1.0, fix32_ceil_to_double(negative), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(2.0, fix32_round_to_double(positive_half), 0.0);
    TEST_EXPECT_DOUBLE_NEAR(-2.0, fix32_round_to_double(negative_half), 0.0);

    TEST_EXPECT_EQ(fix32_from_int(1), fix32_floor(positive));
    TEST_EXPECT_EQ(fix32_from_int(-2), fix32_floor(negative));
    TEST_EXPECT_EQ(fix32_from_int(2), fix32_ceil(positive));
    TEST_EXPECT_EQ(fix32_from_int(-1), fix32_ceil(negative));
    TEST_EXPECT_EQ(fix32_from_int(2), fix32_round(positive_half));
    TEST_EXPECT_EQ(fix32_from_int(-2), fix32_round(negative_half));
    TEST_EXPECT_EQ(fix32_from_int(1), fix32_round(below_positive_half));
    TEST_EXPECT_EQ(fix32_from_int(-1), fix32_round(above_negative_half));
    TEST_EXPECT_EQ(fix32_from_int(2), fix32_floor(fix32_from_int(2)));
    TEST_EXPECT_EQ(fix32_from_int(-2), fix32_ceil(fix32_from_int(-2)));
    TEST_EXPECT_EQ(fix32_from_int(2), fix32_round(fix32_from_int(2)));

    TEST_EXPECT_EQ(1, FIX32_ITRUNC(positive));
    TEST_EXPECT_EQ(-2, FIX32_IFLOOR(negative));
    TEST_EXPECT_EQ(2, FIX32_IROUND(positive_half));
}
