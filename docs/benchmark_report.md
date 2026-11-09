# Benchmarks (report)

This report records the current fix32 0.3.0 implementation and benchmark suite
on November 7, 2026. It includes the corrected rounded constructors, unsigned
integer rounding, high-bit input generation, repeated timing trials, and the
optional `FIX32_USE_SIGNED_SHIFT_DIV` path. The input and measurement changes
make this a new baseline rather than a controlled comparison with the previous
report.

## Run configuration

Measurements used an AMD Ryzen 7 8845HS, Linux 7.0.0-38-generic on x86-64, and
GCC 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1). The CPU governor was `performance`,
with frequency boost enabled. Benchmark processes ran sequentially, pinned to
logical CPU 2; frequency was not locked and the host was not isolated from
other system activity.

The canonical build uses C99, `-Wall -Wextra -Wpedantic`, `CFLAGS=-O2`, and
`BENCH_CFLAGS=-O3`, making `-O3` the effective optimization level. It enables
`FIX32_STATIC_INLINE` and uses the compiler's default architecture target,
without `-march=native`, `-ffast-math`, or LTO. The numeric configuration is:

| Macro | Value |
| --- | --- |
| `FIX32_FRACTIONAL_BITS` | `16` |
| `FIX32_USE_64_BIT` | `1` |
| `FIX32_USE_ARITHMETIC_SHIFT_FLOOR` | `1` |
| `FIX32_USE_SIGNED_SHIFT_MUL` | `0` |
| `FIX32_USE_SIGNED_SHIFT_DIV` | `0` |

The sample count is 8192 and the repeat count is 100. Each scalar and
multiplication row therefore measures 819200 operations per trial. Graphics
rows use 100 repetitions of their displayed pixel count per trial.

Every fixed/float pair receives a one-repeat warm-up and five measured trials,
alternating execution order. Tables show the median time on each side and
the ratio of those medians. The line beneath each row records the maximum
minus minimum trial time. Ratios use the unrounded timings, so dividing the
displayed two-decimal times can give a slightly different ratio.

The canonical run and graph can be reproduced on this host with:

```sh
make -B -j4 all test
taskset -c 2 ./build/benchmark 8192 100 > build/benchmark-output.txt
python3 tools/benchmark_graph.py build/benchmark-output.txt \
    -o docs/benchmark_summary.svg \
    --source-label "docs/benchmark_report.md (2026-11-07)"
```

## Release validation

All 69 test executables passed in each of the following builds. Each run
includes 68 arithmetic/conversion executables with 5,447,097 counted
assertions, plus the benchmark divisor-coverage and trial-order test.

| Build | Result |
| --- | --- |
| Default `-O2` | Passed |
| `-O3 -flto -Werror` | Passed |
| `-O1 -g -fsanitize=undefined,float-cast-overflow -fno-sanitize-recover=all` | Passed |
| `-O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all` | Passed |

The suite covers external and static inline usage, portable floor/ceil,
no-rounding mode, signed-shift multiplication, and signed-shift division with
both wide and narrow intermediates. Boundary tests exercise fractional-bit
counts 1 through 30 in both arithmetic profiles. Signed-shift division tests
keep numerators nonnegative, as required by that option's input contract.

C++11 smoke checks also passed for C-linkage external functions and static
inline use, with signed-shift division enabled in both wide and narrow
profiles. These checks and all timing runs were performed on x86-64; narrow
intermediates were exercised on this host rather than on native 32-bit or
embedded hardware.

Equivalent instrumented test builds can be run with:

```sh
make -j4 test BUILD_DIR=build/test-lto CFLAGS="-O3 -flto -Werror"
make -j4 test BUILD_DIR=build/test-ubsan \
    CFLAGS="-O1 -g -fsanitize=undefined,float-cast-overflow -fno-sanitize-recover=all"
make -j4 test BUILD_DIR=build/test-asan \
    CFLAGS="-O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all"
```

## Raw output

```text
timing: median of 5 trials, one warm-up, alternating order
scalar benchmarks
samples=8192 repeats=100 operations-per-case=819200
operation                   fixed ns/op     base ns/op   fixed/base
int -> representation              0.41           0.62         0.67
  trial range: fixed 60.000 us, base 12.000 us (max - min)
rational -> fixed                  1.44           0.79         1.83
  trial range: fixed 11.000 us, base 3.000 us (max - min)
float -> fixed trunc               0.50           0.61         0.82
  trial range: fixed 5.000 us, base 4.000 us (max - min)
float -> fixed round               0.78           0.62         1.26
  trial range: fixed 30.000 us, base 9.000 us (max - min)
double -> fixed trunc              0.51           0.62         0.82
  trial range: fixed 5.000 us, base 9.000 us (max - min)
double -> fixed round              0.83           0.62         1.33
  trial range: fixed 19.000 us, base 7.000 us (max - min)
sum                                0.24           0.62         0.39
  trial range: fixed 96.000 us, base 6.000 us (max - min)
subtract                           0.36           0.63         0.58
  trial range: fixed 11.000 us, base 8.000 us (max - min)
lerp                               1.05           0.63         1.67
  trial range: fixed 15.000 us, base 6.000 us (max - min)
multiply                           0.76           0.63         1.21
  trial range: fixed 8.000 us, base 9.000 us (max - min)
multiply by int                    0.43           0.63         0.69
  trial range: fixed 12.000 us, base 5.000 us (max - min)
reciprocal by int                  1.26           1.15         1.09
  trial range: fixed 8.000 us, base 11.000 us (max - min)
reciprocal -> fixed                1.46           0.86         1.71
  trial range: fixed 23.000 us, base 61.000 us (max - min)
divide by int                      1.31           0.62         2.10
  trial range: fixed 8.000 us, base 5.000 us (max - min)
divide                             1.44           0.62         2.34
  trial range: fixed 9.000 us, base 5.000 us (max - min)
mul reciprocal                     0.76           0.63         1.20
  trial range: fixed 19.000 us, base 13.000 us (max - min)
trunc -> int                       0.42           0.50         0.84
  trial range: fixed 3.000 us, base 19.000 us (max - min)
ceil -> int                        0.42           0.61         0.69
  trial range: fixed 8.000 us, base 10.000 us (max - min)
floor -> int                       0.36           0.61         0.60
  trial range: fixed 6.000 us, base 6.000 us (max - min)
round -> int                       0.68           0.78         0.88
  trial range: fixed 10.000 us, base 36.000 us (max - min)
floor -> fixed                     0.42           0.94         0.45
  trial range: fixed 11.000 us, base 23.000 us (max - min)
ceil -> fixed                      0.61           0.96         0.63
  trial range: fixed 11.000 us, base 38.000 us (max - min)
round -> fixed                     0.63           1.12         0.57
  trial range: fixed 72.000 us, base 76.000 us (max - min)
value -> float                     0.63           0.63         1.00
  trial range: fixed 9.000 us, base 10.000 us (max - min)
ceil -> float                      0.62           0.93         0.67
  trial range: fixed 12.000 us, base 75.000 us (max - min)
floor -> float                     0.66           0.94         0.71
  trial range: fixed 5.000 us, base 24.000 us (max - min)
round -> float                     0.73           1.12         0.65
  trial range: fixed 23.000 us, base 136.000 us (max - min)
value -> double                    0.63           0.63         1.00
  trial range: fixed 6.000 us, base 8.000 us (max - min)
ceil -> double                     0.62           0.81         0.77
  trial range: fixed 4.000 us, base 29.000 us (max - min)
floor -> double                    0.66           0.81         0.82
  trial range: fixed 6.000 us, base 70.000 us (max - min)
round -> double                    0.65           1.03         0.63
  trial range: fixed 53.000 us, base 180.000 us (max - min)

multiply throughput and dependency benchmarks
operation                   fixed ns/op     base ns/op   fixed/base
multiply array                     0.76           0.09         8.62
  trial range: fixed 16.000 us, base 3.000 us (max - min)
multiply/add chain                 1.68           1.26         1.34
  trial range: fixed 27.000 us, base 23.000 us (max - min)

DDA line benchmarks
repeats-per-case=100
line length        pixels fixed ns/pixel float ns/pixel  fixed/float
32                     33           0.61           1.82         0.33
  trial range: fixed 0.000 us, base 1.000 us (max - min)
128                   129           0.47           1.78         0.26
  trial range: fixed 0.000 us, base 0.000 us (max - min)
512                   513           0.43           1.77         0.24
  trial range: fixed 1.000 us, base 5.000 us (max - min)
2048                 2049           0.42           1.79         0.23
  trial range: fixed 8.000 us, base 5.000 us (max - min)
8192                 8193           0.42           1.76         0.24
  trial range: fixed 20.000 us, base 28.000 us (max - min)

sprite scaler benchmarks
repeats-per-case=100
case                              dest pixels fixed ns/pixel float ns/pixel  fixed/float
16x16 -> 24x24 (1.5x)                     576           0.17           0.89         0.20
  trial range: fixed 0.000 us, base 1.000 us (max - min)
32x24 -> 64x42 (2.0x/1.75x)              2688           0.15           0.85         0.18
  trial range: fixed 1.000 us, base 9.000 us (max - min)
64x48 -> 160x96 (2.5x/2.0x)             15360           0.15           0.84         0.17
  trial range: fixed 8.000 us, base 19.000 us (max - min)
96x64 -> 288x160 (3.0x/2.5x)            46080           0.15           0.83         0.18
  trial range: fixed 9.000 us, base 21.000 us (max - min)
128x96 -> 512x384 (4.0x)               196608           0.15           0.84         0.17
  trial range: fixed 11.000 us, base 81.000 us (max - min)
160x128 -> 1024x768 (6.4x)             786432           0.15           0.83         0.17
  trial range: fixed 30.000 us, base 1890.000 us (max - min)
256x192 -> 2048x1536 (8.0x)           3145728           0.14           0.83         0.17
  trial range: fixed 784.000 us, base 1622.000 us (max - min)
512x384 -> 4096x3072 (8.0x)          12582912           0.15           0.84         0.17
  trial range: fixed 9156.000 us, base 14398.000 us (max - min)
1024x768 -> 8192x6144 (8.0x)         50331648           0.14           0.84         0.17
  trial range: fixed 15355.000 us, base 77707.000 us (max - min)

rotoscaler benchmarks (incremental inverse mapping)
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.75           1.33         0.57
  trial range: fixed 6.000 us, base 6.000 us (max - min)
64x48 -> 160x120 @ 30 deg                   19200           0.75           1.32         0.57
  trial range: fixed 1925.000 us, base 71.000 us (max - min)
96x64 -> 288x192 @ 45 deg                   55296           0.77           1.35         0.57
  trial range: fixed 47.000 us, base 55.000 us (max - min)
128x96 -> 512x384 @ 22.5 deg               196608           0.77           1.35         0.57
  trial range: fixed 1260.000 us, base 1541.000 us (max - min)
256x192 -> 1024x768 @ 60 deg               786432           0.77           1.38         0.56
  trial range: fixed 2210.000 us, base 2485.000 us (max - min)

rotoscaler benchmarks (direct inverse mapping)
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.91           0.93         0.98
  trial range: fixed 4.000 us, base 4.000 us (max - min)
64x48 -> 160x120 @ 30 deg                   19200           0.93           0.92         1.01
  trial range: fixed 8.000 us, base 18.000 us (max - min)
96x64 -> 288x192 @ 45 deg                   55296           0.93           0.91         1.03
  trial range: fixed 34.000 us, base 1699.000 us (max - min)
128x96 -> 512x384 @ 22.5 deg               196608           0.95           0.91         1.04
  trial range: fixed 313.000 us, base 1635.000 us (max - min)
256x192 -> 1024x768 @ 60 deg               786432           0.97           0.94         1.03
  trial range: fixed 9711.000 us, base 6624.000 us (max - min)
```

## Benchmark item reference

Each scalar row compares a fixed-point operation with the corresponding
floating-point or base representation loop shown by the `base ns/op` column.
The ratio is `fixed ns/op / base ns/op`; values below `1.0` mean the fixed
path was faster in that run, while values above `1.0` mean the base path was
faster. The workload sections use the same ratio idea but report time per
pixel instead of time per scalar operation.

| Item | Meaning |
| --- | --- |
| `int -> representation` | Convert signed integer inputs to the native representation: `fix32_from_int()` for fixed point and a cast to `float` for the baseline. |
| `rational -> fixed` | Convert integer numerator/denominator pairs to fixed point, compared with float division followed by truncating float-to-fixed conversion. |
| `float -> fixed trunc` | Convert `float` inputs to fixed point with truncation toward zero, compared with reading and accumulating the float values. |
| `float -> fixed round` | Convert `float` inputs to fixed point with nearest-value rounding, compared with the same float baseline loop. |
| `double -> fixed trunc` | Convert `double` inputs to fixed point with truncation toward zero, compared with reading and accumulating the double values. |
| `double -> fixed round` | Convert `double` inputs to fixed point with nearest-value rounding, compared with the same double baseline loop. |
| `sum` | Repeatedly add small values into an accumulator using `fix32_add()` or float addition. |
| `subtract` | Subtract one input stream from another using `fix32_sub()` or float subtraction. |
| `lerp` | Interpolate between two values with `fix32_lerp()`, compared with the equivalent float expression. |
| `multiply` | Multiply two fixed-point values with `fix32_mul()`, compared with float multiplication. |
| `multiply by int` | Multiply a fixed-point value by a small integer with `fix32_mul_by_int()`, compared with multiplying a float by the same integer. |
| `reciprocal by int` | Build a fixed-point reciprocal from an integer divisor, compared with float reciprocal computation converted back to fixed point. |
| `reciprocal -> fixed` | Build a reciprocal from a fixed-point divisor with `fix32_reciprocal()`, compared with a float reciprocal converted back to fixed point. |
| `divide by int` | Divide a fixed-point value by an integer with `fix32_div_by_int()`, compared with float division by the same integer. |
| `divide` | Divide one fixed-point value by another with `fix32_div()`, compared with float division. |
| `mul reciprocal` | Multiply by a precomputed reciprocal, compared with the same operation in float. |
| `trunc -> int` | Convert to integer by truncating toward zero. |
| `ceil -> int` | Convert to integer with ceiling semantics. |
| `floor -> int` | Convert to integer with floor semantics. |
| `round -> int` | Convert to integer with nearest-value rounding. |
| `floor -> fixed` | Floor the fractional part while keeping the result in fixed-point representation, compared with the float helper returning a float-valued integer. |
| `ceil -> fixed` | Ceil the fractional part while keeping the result in fixed-point representation, compared with the float helper returning a float-valued integer. |
| `round -> fixed` | Round while keeping the result in fixed-point representation, compared with the float helper returning a float-valued integer. |
| `value -> float` | Convert fixed-point values to `float`, compared with reading and accumulating existing float values. |
| `ceil -> float` | Apply fixed-point ceiling and return `float`, compared with float ceiling logic returning `float`. |
| `floor -> float` | Apply fixed-point floor and return `float`, compared with float floor logic returning `float`. |
| `round -> float` | Apply fixed-point rounding and return `float`, compared with float rounding logic returning `float`. |
| `value -> double` | Convert fixed-point values to `double`, compared with reading and accumulating existing double values. |
| `ceil -> double` | Apply fixed-point ceiling and return `double`, compared with double ceiling logic returning `double`. |
| `floor -> double` | Apply fixed-point floor and return `double`, compared with double floor logic returning `double`. |
| `round -> double` | Apply fixed-point rounding and return `double`, compared with double rounding logic returning `double`. |
| `multiply array` | Multiply ordinary input arrays into 32-bit output arrays, with a final batch checksum and an indirect call per batch. |
| `multiply/add chain` | Repeatedly evaluate a bounded dependent multiply/add recurrence, storing only the final value. |
| DDA `line length` rows | Run an arithmetic-only DDA line stepper for the requested major-axis length. The `pixels` column is the number of generated points. |
| Sprite scaler `case` rows | Run nearest-neighbor-style source coordinate generation from the source dimensions to the destination dimensions shown in the label. |
| Rotoscaler incremental `case` rows | Run affine inverse mapping by incrementally advancing source coordinates across each row and down each column. |
| Rotoscaler direct `case` rows | Run affine inverse mapping by recomputing the source coordinate from destination `x` and `y` for every pixel. |

## Results

The original scalar loops include volatile loads and checksum accumulation.
Their fixed and float sides sometimes use different checksum types, so these
rows measure the complete loop rather than isolated primitive latency.
Fixed addition, masking, and integer-facing rounding remain favorable in this
run: repeated addition has a fixed/base ratio of `0.39`, fixed-valued floor
`0.45`, and integer rounding `0.88`. Fixed multiplication has a ratio of
`1.21`, and fixed division `2.34`. Construction rows compare conversion work
with an identity baseline; their ratios are not a comparison between two
equivalent conversion algorithms.

The separate multiplication section exposes a larger throughput difference.
The ordinary array kernels take `0.76 ns/op` for fixed and `0.09 ns/op` for
float, with a measured ratio of `8.62`. Their inner loops write results without
per-element checksum accumulation and permit vectorization. The dependent
multiply/add chain takes `1.68 ns/op` for fixed and `1.26 ns/op` for float,
a ratio of `1.34`. The chain includes both multiplication and addition;
it does not isolate multiplication latency. Both rows include the batch or
final-result overhead described in [benchmark_details.md](benchmark_details.md).

The graphics workloads favor fixed point when their inner loops repeatedly
step coordinates and convert them to integer sample positions. The summary
graph uses an unweighted arithmetic mean of the printed case ratios:

| Workload | Mean fixed/float |
| --- | ---: |
| DDA line | `0.26` |
| Sprite scaler | `0.18` |
| Incremental rotoscaler | `0.57` |
| Direct rotoscaler | `1.02` |

For the larger DDA cases, fixed stepping costs about `0.42 ns/pixel` against
`1.76–1.79 ns/pixel` for float. Sprite scaling is also favorable, with case
ratios of `0.17–0.20`. Incremental rotoscaling has ratios of `0.56–0.57`;
direct affine recomputation is close to parity at `0.98–1.04`. Small cases are
sensitive to clock resolution, and a zero reported trial range does not imply
zero uncertainty.

Reciprocal construction costs `1.46 ns/op`, multiplication by a precomputed
reciprocal `0.76 ns/op`, and direct fixed division `1.44 ns/op`. Using these
checksum-inclusive loop timings as a rough estimate, reuse becomes cheaper
at three uses because `1.46 + 0.76 * N < 1.44 * N` when `N >= 3`.
A quantized reciprocal can produce different results from direct division,
so that estimate alone does not establish numerical suitability.

## Additional configurations

The signed-shift multiply, signed-shift divide, narrow arithmetic, external
function, and external function with LTO builds are measured separately from
the canonical run. Their complete console outputs are captured under
`build/benchmark-*-output.txt`. The shifted division row uses nonnegative
numerators on both sides, while the default row exercises both signs.
Signed-shift multiplication can differ from the default by one raw unit for
negative nonintegral products. The narrow profile changes both precision and
scalar input range, so it is not an interchangeable default-format result.

Selected fixed-point median costs are shown below in ns/op. Separate runs can
vary in code layout, compiler optimization, and system conditions; differences
in unchanged operations should not be attributed to the selected macro alone.

| Configuration | Multiply | Divide | Round to int | Multiply array | Multiply/add chain |
| --- | ---: | ---: | ---: | ---: | ---: |
| Default inline | 0.76 | 1.44 | 0.68 | 0.76 | 1.68 |
| Signed-shift multiply | 0.56 | 1.55 | 0.82 | 0.56 | 1.28 |
| Signed-shift divide | 0.76 | 1.46 | 0.85 | 0.76 | 1.67 |
| Narrow 8-bit fraction | 0.67 | 1.27 | 0.64 | 0.19 | 1.47 |
| External functions | 1.33 | 1.59 | 1.85 | 1.26 | 2.09 |
| External with LTO | 0.77 | 1.45 | 0.72 | 0.76 | 1.66 |

The following unweighted workload means use fixed/float ratios, rather than
absolute times:

| Configuration | DDA | Sprite scaler | Incremental rotoscaler | Direct rotoscaler |
| --- | ---: | ---: | ---: | ---: |
| Default inline | 0.26 | 0.18 | 0.57 | 1.02 |
| Signed-shift multiply | 0.26 | 0.17 | 0.57 | 1.02 |
| Signed-shift divide | 0.26 | 0.18 | 0.57 | 1.03 |
| Narrow 8-bit fraction | 0.54 | 0.17 | 0.50 | 0.78 |
| External functions | 2.18 | 2.00 | 2.81 | 6.23 |
| External with LTO | 0.26 | 0.18 | 0.57 | 1.02 |

External function calls are material in these hot loops on this compiler and
CPU. LTO brings the external build close to the inline build. The signed-shift
division option remains supported, with its nonnegative-numerator contract;
its division row and the default row have different numerator domains, so the
numbers are not an instruction-only A/B comparison.

All supplementary runs used CPU 2, 8192 samples, 100 repeats, and the same
five-trial timing method. The inline shifted and narrow builds come from the
normal Makefile targets. The narrow target uses `FIX32_FRACTIONAL_BITS=8`,
`FIX32_USE_64_BIT=0`, and `BENCHMARK_NARROW_INPUTS=1`. External builds clear
`BENCH_CPPFLAGS`; the LTO build additionally sets `BENCH_CFLAGS="-O3 -flto"`.
The supplementary runs can be reproduced with:

```sh
make -j4 benchmark-shift-mul benchmark-shift-div benchmark-32-bit
make benchmark BUILD_DIR=build/external BENCH_CPPFLAGS=
make benchmark BUILD_DIR=build/lto BENCH_CPPFLAGS= BENCH_CFLAGS="-O3 -flto"
taskset -c 2 ./build/benchmark-shift-mul 8192 100 > build/benchmark-shift-mul-output.txt
taskset -c 2 ./build/benchmark-shift-div 8192 100 > build/benchmark-shift-div-output.txt
taskset -c 2 ./build/benchmark-32-bit 8192 100 > build/benchmark-32-bit-output.txt
taskset -c 2 ./build/external/benchmark 8192 100 > build/benchmark-external-output.txt
taskset -c 2 ./build/lto/benchmark 8192 100 > build/benchmark-lto-output.txt
```

## Scope and interpretation

The DDA, scaler, and rotoscaler cases compute coordinates and checksums. They
do not fetch textures, write framebuffers, clip, blend, or filter pixels, and
they do not check equality of fixed and float sample coordinates. Quantized
steps can select different texels near boundaries. Setup is performed once
before a batch of repetitions, so per-draw setup costs are heavily amortized.

These results support measuring fixed point in bounded, integer-facing
stepping loops. They also show why volatile-load scalar timings should not
be generalized to bulk arithmetic throughput. Memory access, SIMD choices,
compiler optimization, rounding policy, and input ranges all affect the
comparison. The library remains unchecked: callers must respect result and
intermediate ranges and pass nonzero divisors. The optional signed-shift
division path remains available for workloads that satisfy its documented
nonnegative-numerator contract.
