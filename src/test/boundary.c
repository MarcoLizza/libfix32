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

#include <stddef.h>

void test_boundaries(void)
{
    const float scale_float = (float)FIX32_ONE;
    const double scale_double = (double)FIX32_ONE;
    const int32_t raw_values[] = {
        INT32_MIN, INT32_MIN + 1, INT32_MIN + FIX32_HALF,
        -FIX32_ONE, -FIX32_HALF - 1, -FIX32_HALF, -FIX32_HALF + 1,
        -1, 0, 1, FIX32_HALF - 1, FIX32_HALF, FIX32_HALF + 1,
        FIX32_ONE, INT32_MAX - FIX32_HALF, INT32_MAX - 1, INT32_MAX
    };
    uint32_t state = UINT32_C(0x9e3779b9);
    size_t index;

    // Adjacent IEEE-754 values around half a raw unit must stay on their side
    // of the tie. Hexadecimal literals keep these boundaries exact.
    TEST_EXPECT_EQ(0, fix32_round_from_float(0x1.fffffep-2f / scale_float));
    TEST_EXPECT_EQ(0, fix32_round_from_float(-0x1.fffffep-2f / scale_float));
    TEST_EXPECT_EQ(1, fix32_round_from_float(0.5f / scale_float));
    TEST_EXPECT_EQ(-1, fix32_round_from_float(-0.5f / scale_float));
    TEST_EXPECT_EQ(1, fix32_round_from_float(0x1.000002p-1f / scale_float));
    TEST_EXPECT_EQ(-1, fix32_round_from_float(-0x1.000002p-1f / scale_float));
    TEST_EXPECT_EQ(0, fix32_round_from_double(0x1.fffffffffffffp-2 / scale_double));
    TEST_EXPECT_EQ(0, fix32_round_from_double(-0x1.fffffffffffffp-2 / scale_double));
    TEST_EXPECT_EQ(1, fix32_round_from_double(0.5 / scale_double));
    TEST_EXPECT_EQ(-1, fix32_round_from_double(-0.5 / scale_double));
    TEST_EXPECT_EQ(1, fix32_round_from_double(0x1.0000000000001p-1 / scale_double));
    TEST_EXPECT_EQ(-1, fix32_round_from_double(-0x1.0000000000001p-1 / scale_double));

    // At this magnitude a float bias of 0.5 can alter an exact raw integer.
    TEST_EXPECT_EQ(8388609, fix32_round_from_float(8388609.0f / scale_float));
    TEST_EXPECT_EQ(-8388609, fix32_round_from_float(-8388609.0f / scale_float));
    TEST_EXPECT_EQ(INT32_MIN, fix32_round_from_float(-0x1p31f / scale_float));
    TEST_EXPECT_EQ(2147483520, fix32_round_from_float(0x1.fffffep30f / scale_float));
    TEST_EXPECT_EQ(INT32_MAX,
                   fix32_round_from_double((double)INT32_MAX / scale_double));
    TEST_EXPECT_EQ(INT32_MIN,
                   fix32_round_from_double((double)INT32_MIN / scale_double));
    TEST_EXPECT_EQ(INT32_MAX,
                   fix32_round_from_double(((double)INT32_MAX + 0.25) / scale_double));
    TEST_EXPECT_EQ(INT32_MIN,
                   fix32_round_from_double(((double)INT32_MIN - 0.25) / scale_double));

    TEST_EXPECT_EQ(INT32_MIN, fix32_from_int(FIX32_INT_MIN));
    TEST_EXPECT_EQ((int64_t)FIX32_INT_MAX * FIX32_ONE,
                   fix32_from_int(FIX32_INT_MAX));
    TEST_EXPECT_EQ(FIX32_ONE, fix32_from_rational(1, 1));
    TEST_EXPECT_EQ(-FIX32_ONE, fix32_from_rational(-1, 1));
    TEST_EXPECT_EQ(1, fix32_div(1, FIX32_ONE));
    TEST_EXPECT_EQ(-1, fix32_div(1, -FIX32_ONE));
#if !FIX32_USE_SIGNED_SHIFT_DIV
    TEST_EXPECT_EQ(-1, fix32_div(-1, FIX32_ONE));
    TEST_EXPECT_EQ(1, fix32_div(-1, -FIX32_ONE));
#endif

    for (index = 0; index < 10000 + sizeof(raw_values) / sizeof(raw_values[0]); ++index) {
        int32_t raw;
        int64_t nearest;
        int32_t whole;
        int32_t remainder;

        if (index < sizeof(raw_values) / sizeof(raw_values[0])) {
            raw = raw_values[index];
        } else {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            raw = (int32_t)((int64_t)state + INT32_MIN);
        }
        nearest = (int64_t)raw + ((raw < 0) ? -(int64_t)FIX32_HALF : FIX32_HALF);
        nearest /= FIX32_ONE;
        whole = raw / FIX32_ONE;
        remainder = raw % FIX32_ONE;

        TEST_EXPECT_EQ(nearest, fix32_round_to_int(raw));
        TEST_EXPECT_EQ(whole, fix32_trunc_to_int(raw));
        TEST_EXPECT_EQ(whole - (remainder < 0), fix32_floor_to_int(raw));
        TEST_EXPECT_EQ(whole + (remainder > 0), fix32_ceil_to_int(raw));
        TEST_EXPECT_DOUBLE_NEAR((double)raw / scale_double, fix32_to_double(raw), 0.0);
        TEST_EXPECT_EQ(raw, fix32_from_double(fix32_to_double(raw)));
        TEST_EXPECT_EQ(raw, fix32_round_from_double(fix32_to_double(raw)));
        if (nearest >= FIX32_INT_MIN && nearest <= FIX32_INT_MAX) {
            TEST_EXPECT_EQ(nearest * FIX32_ONE, fix32_round(raw));
        }
    }
}
