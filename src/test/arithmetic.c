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

void test_arithmetic(void)
{
    const fix32_t one_and_a_half = fix32_from_float(1.5f);
    const fix32_t two_and_a_quarter = fix32_from_float(2.25f);

    TEST_EXPECT_EQ(fix32_from_float(0.75f),
                   fix32_add(fix32_from_float(1.25f),
                             fix32_from_float(-0.5f)));
    TEST_EXPECT_EQ(fix32_from_float(-3.0f),
                   fix32_add(fix32_from_float(-1.25f),
                             fix32_from_float(-1.75f)));
    TEST_EXPECT_EQ(fix32_from_float(1.75f),
                   fix32_sub(fix32_from_float(2.25f),
                             fix32_from_float(0.5f)));
    TEST_EXPECT_EQ(fix32_from_float(-1.75f),
                   fix32_sub(fix32_from_float(-1.25f),
                             fix32_from_float(0.5f)));

    TEST_EXPECT_EQ(fix32_from_float(3.375f),
                   fix32_mul(one_and_a_half, two_and_a_quarter));
    TEST_EXPECT_EQ(fix32_from_float(-3.0f),
                   fix32_mul(one_and_a_half, fix32_from_int(-2)));
    TEST_EXPECT_EQ(0, fix32_mul(0, fix32_from_int(100)));
    TEST_EXPECT_EQ(fix32_from_float(4.5f),
                   fix32_mul_by_int(one_and_a_half, 3));
    TEST_EXPECT_EQ(fix32_from_float(-3.0f),
                   fix32_mul_by_int(one_and_a_half, -2));

    TEST_EXPECT_EQ(fix32_from_float(1.5f),
                   fix32_div_by_int(fix32_from_int(3), 2));
    TEST_EXPECT_EQ(fix32_from_float(-1.5f),
                   fix32_div_by_int(fix32_from_int(-3), 2));

    TEST_EXPECT_EQ(fix32_from_int(3),
                   fix32_div(fix32_from_float(7.5f),
                             fix32_from_float(2.5f)));
    TEST_EXPECT_EQ(fix32_from_float(-2.5f),
                   fix32_div(fix32_from_int(5), fix32_from_int(-2)));
    TEST_EXPECT_EQ(FIX32_ONE / 3,
                   fix32_div(fix32_from_int(1), fix32_from_int(3)));

    TEST_EXPECT_EQ(FIX32_ONE / 4, fix32_reciprocal_by_int(4));
    TEST_EXPECT_EQ(-(FIX32_ONE / 2), fix32_reciprocal_by_int(-2));
    TEST_EXPECT_EQ(fix32_from_float(0.25f),
                   fix32_reciprocal(fix32_from_int(4)));
    TEST_EXPECT_EQ(fix32_from_float(-0.5f),
                   fix32_reciprocal(fix32_from_int(-2)));
    TEST_EXPECT_EQ(fix32_from_int(1),
                   fix32_reciprocal(fix32_from_int(1)));
}
