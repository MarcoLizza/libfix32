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

#ifndef FPMATH_H
#define FPMATH_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

typedef int32_t fix32_t;

#ifndef FIX32_FRACTION_BITS
#define FIX32_FRACTION_BITS 16
#endif

#if (FIX32_FRACTION_BITS < 1) || (FIX32_FRACTION_BITS > 30)
#error "FIX32_FRACTION_BITS must be between 1 and 30"
#endif

#define FIX32_SCALE ((int32_t)1 << FIX32_FRACTION_BITS)
#define FIX32_HALF (FIX32_SCALE / 2)
#define FIX32_FRAC_MASK (FIX32_SCALE - 1)
#define FIX32_MAX_INT_INPUT ((int32_t)(INT32_MAX / FIX32_SCALE))
#define FIX32_MIN_INT_INPUT ((int32_t)(INT32_MIN / FIX32_SCALE))

/*
 * Enable runtime assertions in debug builds to catch values that do not fit
 * the chosen fixed-point format or violate optional multiply-range hints.
 */
#ifndef FIX32_ENABLE_DEBUG_CHECKS
#ifdef NDEBUG
#define FIX32_ENABLE_DEBUG_CHECKS 0
#else
#define FIX32_ENABLE_DEBUG_CHECKS 1
#endif
#endif

#if FIX32_ENABLE_DEBUG_CHECKS
#define FIX32_ASSERT(expression) assert(expression)
#else
#define FIX32_ASSERT(expression) ((void)0)
#endif

/*
 * Set to 0 before including this header to use the portable floor-to-int path.
 * The default uses an arithmetic right shift for speed, which is
 * implementation-defined for negative signed integers in C99.
 */
#ifndef FIX32_USE_ARITHMETIC_SHIFT_FLOOR
#define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 1
#endif

/*
 * Set to 1 before including this header to scale division numerators with a
 * signed left shift. This invokes undefined behavior when the numerator is
 * negative. The default multiplication path is defined for every fix32_t
 * value and is typically optimized to the same machine instruction.
 */
#ifndef FIX32_USE_SIGNED_SHIFT_DIV
#define FIX32_USE_SIGNED_SHIFT_DIV 0
#endif

/*
 * Optional hint: define the maximum magnitude bits expected to the left of the
 * radix point in operands passed to fix32_mul(). This does not change the
 * stored fixed-point format; it only describes the operand range the
 * programmer promises to use for auto-selecting the multiply path.
 *
 * If this hint is not provided, the header falls back to the safe 64-bit
 * multiply path by default.
 */
#ifndef FIX32_USE_64BIT_MUL
#ifdef FIX32_MUL_INTEGER_BITS
#define FIX32_USE_64BIT_MUL \
    (((FIX32_MUL_INTEGER_BITS + FIX32_FRACTION_BITS) > 15) ? 1 : 0)
#else
#define FIX32_USE_64BIT_MUL 1
#endif
#endif

static inline void fix32_assert_raw_range(int64_t raw_value)
{
#if FIX32_ENABLE_DEBUG_CHECKS
    FIX32_ASSERT(raw_value >= (int64_t)INT32_MIN);
    FIX32_ASSERT(raw_value <= (int64_t)INT32_MAX);
#else
    (void)raw_value;
#endif
}

static inline void fix32_assert_mul_hint(fix32_t value)
{
#if FIX32_ENABLE_DEBUG_CHECKS
#ifdef FIX32_MUL_INTEGER_BITS
    const int64_t limit =
        (int64_t)1 << (FIX32_MUL_INTEGER_BITS + FIX32_FRACTION_BITS);

    FIX32_ASSERT((int64_t)value >= -limit);
    FIX32_ASSERT((int64_t)value < limit);
#else
    (void)value;
#endif
#else
    (void)value;
#endif
}

static inline fix32_t fix32_from_raw(int32_t raw_value)
{
    return raw_value;
}

static inline int32_t fix32_raw(fix32_t value)
{
    return value;
}

static inline fix32_t fix32_from_int(int32_t value)
{
    const int64_t scaled = (int64_t)value * (int64_t)FIX32_SCALE;

    FIX32_ASSERT(value >= FIX32_MIN_INT_INPUT);
    FIX32_ASSERT(value <= FIX32_MAX_INT_INPUT);
    fix32_assert_raw_range(scaled);
    return (fix32_t)scaled;
}

static inline fix32_t fix32_from_float(float value)
{
    const float scaled = value * (float)FIX32_SCALE;

    FIX32_ASSERT(scaled >= ((float)INT32_MIN - 0.5f));
    FIX32_ASSERT(scaled <= ((float)INT32_MAX + 0.5f));
    return (fix32_t)(scaled + ((scaled >= 0.0f) ? 0.5f : -0.5f));
}

static inline fix32_t fix32_add(fix32_t left, fix32_t right)
{
#if FIX32_ENABLE_DEBUG_CHECKS
    fix32_assert_raw_range((int64_t)left + (int64_t)right);
#endif
    return left + right;
}

static inline fix32_t fix32_mul(fix32_t left, fix32_t right)
{
    fix32_assert_mul_hint(left);
    fix32_assert_mul_hint(right);
#if FIX32_USE_64BIT_MUL
    const int64_t scaled_product =
        ((int64_t)left * (int64_t)right) >> FIX32_FRACTION_BITS;

    /* Multiply in 64 bits, then scale the product back down to the fixed format. */
    fix32_assert_raw_range(scaled_product);
    return (fix32_t)scaled_product;
#else
    /*
     * The caller must keep operands small enough that the 32-bit product fits
     * before the final shift.
     */
#if FIX32_ENABLE_DEBUG_CHECKS
    fix32_assert_raw_range((int64_t)left * (int64_t)right);
#endif
    return (fix32_t)((left * right) >> FIX32_FRACTION_BITS);
#endif
}

static inline fix32_t fix32_div_by_int(fix32_t numerator, int32_t denominator)
{
    FIX32_ASSERT(denominator != 0);
#if FIX32_ENABLE_DEBUG_CHECKS
    fix32_assert_raw_range((int64_t)numerator / (int64_t)denominator);
#endif
    return numerator / denominator;
}

static inline fix32_t fix32_div(fix32_t numerator, fix32_t denominator)
{
#if FIX32_USE_SIGNED_SHIFT_DIV
    const int64_t scaled_numerator =
        (int64_t)numerator << FIX32_FRACTION_BITS;
#else
    const int64_t scaled_numerator =
        (int64_t)numerator * (int64_t)FIX32_SCALE;
#endif
    int64_t quotient;

    FIX32_ASSERT(denominator != 0);
    quotient = scaled_numerator / (int64_t)denominator;
    fix32_assert_raw_range(quotient);
    return (fix32_t)quotient;
}

static inline fix32_t fix32_reciprocal(fix32_t value)
{
    return fix32_div(fix32_from_int(1), value);
}

static inline float fix32_to_float(fix32_t value)
{
    return (float)value / (float)FIX32_SCALE;
}

static inline int32_t fix32_floor_to_int(fix32_t value)
{
#if FIX32_USE_ARITHMETIC_SHIFT_FLOOR
    /* Fast path: relies on the target doing arithmetic right shifts for signed values. */
    return value >> FIX32_FRACTION_BITS;
#else
    const int32_t whole = value / FIX32_SCALE;
    const int32_t fractional = value % FIX32_SCALE;

    if (fractional != 0 && value < 0) {
        return whole - 1;
    }

    return whole;
#endif
}

static inline int32_t fix32_ceil_to_int(fix32_t value)
{
#if FIX32_USE_ARITHMETIC_SHIFT_FLOOR
    const int32_t whole = value >> FIX32_FRACTION_BITS;

    /* The fractional bits live in the low raw bits because the scale is a power of two. */
    return whole + ((value & FIX32_FRAC_MASK) != 0);
#else
    const int32_t whole = value / FIX32_SCALE;
    const int32_t fractional = value % FIX32_SCALE;

    if (fractional != 0 && value > 0) {
        return whole + 1;
    }

    return whole;
#endif
}

static inline int32_t fix32_round_to_int(fix32_t value)
{
    int64_t adjusted = (int64_t)value;

    adjusted += (adjusted >= 0) ? FIX32_HALF : -FIX32_HALF;
    return (int32_t)(adjusted / FIX32_SCALE);
}

static inline float fix32_floor_to_float(fix32_t value)
{
    return (float)fix32_floor_to_int(value);
}

static inline float fix32_ceil_to_float(fix32_t value)
{
    return (float)fix32_ceil_to_int(value);
}

static inline float fix32_round_to_float(fix32_t value)
{
    return (float)fix32_round_to_int(value);
}

#endif
