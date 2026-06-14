# Introduction

`fix32` is a small C99 [single header only](https://en.wikipedia.org/wiki/Header-only) [fixed-point](https://en.wikipedia.org/wiki/Fixed-point_arithmetic) math library.

I started to write this library to support fixed-point math in my game engine (Tofu Engine). Other fixed-point libraries exist, and some of them are quite good, but... well, we all have our own preferences and priorities when it comes to design and implementation, and I wanted to write my own library that fits my specific needs and style.

That said, I didn't want to make it a one-off for my game engine, but generally useful and reusable.

Internally, it uses a signed 32-bit `fix32_t` representation which is a good balance between range and precision in most scenarios. The library also includes a number of configuration options to allow users to customize the behavior and performance characteristics of the library to suit their specific needs.

By no means a comprehensive fixed-point library (see [libfixmath](https://code.google.com/archive/p/libfixmath/) for that purpose), `fix32` provides a basic set of operations and helpers that are commonly needed in game development and other performance-sensitive applications.

It's designed to be portable and configurable, with a focus on performance and ease of use.

## Usage

To use the library, drop the header in your project and simply include it in your C source files:

```c
#include "fix32.h"

// Your code here...
```

As simple as that! The library is designed to be easy to integrate into existing projects without additional setup.

A number of configuration macros are available to customize the behavior and performance characteristics of the library. These can be defined at compile time to enable or disable specific features or optimizations:

- `FIX32_FRACTIONAL_BITS`: fractional precision, default `16`
- `FIX32_USE_ARITHMETIC_SHIFT_FLOOR`: selects the fast floor/ceil path, default
  `1`
- `FIX32_USE_SIGNED_SHIFT_MUL`: enables signed-shift scaling for `fix32_mul()`,
  default `0`
- `FIX32_USE_SIGNED_SHIFT_DIV`: enables signed left-shift scaling for
  `fix32_div()`, default `0`
- `FIX32_USE_64_BIT`: selects wider intermediates for selected operations,
  default `1`
- `FIX32_INTEGER_BITS`: optional hint used to auto-select `FIX32_USE_64_BIT`
- `FIX32_NO_ROUNDING`: switches the `FIX32_FROM_FLOAT`, `FIX32_FROM_DOUBLE`,
  and `FIX32_TO_INT` helper macros to truncating variants

Typical overrides look like this:

```sh
make CFLAGS="-O2 -DFIX32_NO_ROUNDING"
make benchmark BENCH_CFLAGS="-O3 -DFIX32_USE_SIGNED_SHIFT_MUL=1"
```

## Build

```sh
make
```

This builds the default test and benchmark executables under `build/`.

## Test

A comprehensive test suite is included under `tests/`. The tests cover the full range of operations and configurations, including edge cases and error handling.

To run the tests, simply execute:

```sh
make test
```

The test target runs the full suite, including the configuration variants for portable floor/ceil handling, no-rounding mode, 32-bit multiply, and the signed-shift multiply/divide paths.

## Benchmark

```sh
make benchmark
./build/benchmark 8192 100
```

The benchmark compares fixed-point and floating-point work for scalar math, line stepping, sprite scaling, and sprite roto-scaling. The reason for this kind of benchmark is to provide a realistic performance comparison between fixed-point and floating-point math in the context of game development, where these operations are commonly used (or, at least, they are in the context of my game engine :D).

See [docs/benchmark_details.md](docs/benchmark_details.md) for more information.

## Notes

- Design: [docs/design.md](docs/design.md)
- Benchmark details: [docs/benchmark_details.md](docs/benchmark_details.md)
- Benchmark report: [docs/benchmark_report.md](docs/benchmark_report.md)
