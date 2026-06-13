# fpmath benchmark report

This report captures the current default benchmark suite, including all scalar
API benchmarks and the DDA, sprite-scaler, and rotoscaler workloads. The
workload results measure coordinate arithmetic only; they do not include
framebuffer access, texture sampling, blending, clipping, or other renderer
costs.

## Run configuration

The measurements were recorded on June 13, 2026 with:

- fpmath 0.1.0
- AMD Ryzen 7 8845HS
- Ubuntu x86-64, Linux 6.17
- GCC 13.3.0
- C99
- `-O3` as the effective optimization level

Build and execution:

```sh
make -B benchmark CFLAGS="-O2" BENCH_CFLAGS="-O3"
./build/benchmark 8192 100
```

Parameters:

- `sample-count = 8192`
- `repeat-count = 100`
- `819200` operations per scalar row

The default library configuration was:

- `FIX32_FRACTIONAL_BITS = 16`
- `FIX32_USE_64_BIT = 1`
- `FIX32_USE_ARITHMETIC_SHIFT_FLOOR = 1`
- `FIX32_USE_SIGNED_SHIFT_MUL = 0`
- `FIX32_USE_SIGNED_SHIFT_DIV = 0`

The benchmark prepares shared fixed-point inputs with
`fix32_round_from_float()`. The scalar conversion rows separately measure both
the truncating and rounding constructors.

The Makefile also provides `benchmark-shift-mul` and `benchmark-shift-div` for
the optional signed-shift implementations. Their results are not mixed into
this default-configuration report.

This is one benchmark invocation using the suite's `clock()`-based CPU-time
measurement. Small differences should be expected between runs.

## Raw output

```text
scalar benchmarks
samples=8192 repeats=100 operations-per-case=819200
operation                   fixed ns/op     base ns/op   fixed/base
int -> representation              0.36           0.61         0.59
float -> fixed trunc               0.41           0.62         0.66
float -> fixed round               0.70           0.62         1.12
double -> fixed trunc              0.41           0.62         0.66
double -> fixed round              0.65           0.61         1.07
sum                                0.25           0.61         0.41
subtract                           0.36           0.61         0.59
multiply                           0.74           0.61         1.22
multiply by int                    0.41           0.61         0.68
reciprocal by int                  1.23           0.92         1.34
reciprocal -> fixed                1.74           0.86         2.02
divide by int                      1.30           0.66         1.99
divide                             1.44           0.98         1.48
mul reciprocal                     0.74           0.62         1.19
trunc -> int                       0.41           0.50         0.83
ceil -> int                        0.41           1.04         0.40
floor -> int                       0.36           1.04         0.34
round -> int                       0.68           1.59         0.42
floor -> fixed                     0.42           2.53         0.16
ceil -> fixed                      0.60           2.53         0.24
round -> fixed                     0.73           2.56         0.29
value -> float                     0.62           0.62         1.00
ceil -> float                      0.62           2.52         0.25
floor -> float                     0.64           2.55         0.25
round -> float                     0.68           2.65         0.26
value -> double                    0.63           0.65         0.98
ceil -> double                     0.63           0.93         0.68
floor -> double                    0.63           0.88         0.73
round -> double                    0.73           1.21         0.60

DDA line benchmarks
repeats-per-case=100
line length        pixels fixed ns/pixel float ns/pixel  fixed/float
32                     33           1.21           2.73         0.44
128                   129           1.24           2.79         0.44
512                   513           1.19           2.83         0.42
2048                 2049           1.18           2.88         0.41
8192                 8193           1.18           2.82         0.42

sprite scaler benchmarks
repeats-per-case=100
case                              dest pixels fixed ns/pixel float ns/pixel  fixed/float
16x16 -> 24x24 (1.5x)                     576           0.17           2.50         0.07
32x24 -> 64x42 (2.0x/1.75x)              2688           0.15           2.51         0.06
64x48 -> 160x96 (2.5x/2.0x)             15360           0.15           2.54         0.06
96x64 -> 288x160 (3.0x/2.5x)            46080           0.15           2.55         0.06
128x96 -> 512x384 (4.0x)               196608           0.14           2.56         0.06
160x128 -> 1024x768 (6.4x)             786432           0.14           2.58         0.06
256x192 -> 2048x1536 (8.0x)           3145728           0.14           2.59         0.05
512x384 -> 4096x3072 (8.0x)          12582912           0.15           2.59         0.06
1024x768 -> 8192x6144 (8.0x)         50331648           0.14           2.60         0.05

rotoscaler benchmarks
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.77           2.66         0.29
64x48 -> 160x120 @ 30 deg                   19200           0.76           2.69         0.28
96x64 -> 288x192 @ 45 deg                   55296           0.77           2.71         0.28
128x96 -> 512x384 @ 22.5 deg               196608           0.77           2.74         0.28
256x192 -> 1024x768 @ 60 deg               786432           0.77           2.76         0.28
```

## Scalar results

The strongest fixed-point scalar results are the operations built from integer
addition, masking, and integer-facing rounding. Repeated addition costs
`0.25 ns/op` against `0.61 ns/op` for the float baseline. Floor and ceil to an
integer have ratios of `0.34` and `0.40`, while fixed-valued floor, ceil, and
round range from `0.16` to `0.29`.

Truncating float and double construction is cheaper than the corresponding
baseline loop in this run, at a ratio of `0.66`. Explicit rounded construction
is slightly slower than the baseline, with ratios of `1.12` for float and
`1.07` for double. These sub-nanosecond conversion rows are particularly
sensitive to compiler optimization and should not be treated as standalone
architectural evidence.

Fixed multiplication is `1.22` times the float cost under the default
64-bit-product configuration. Multiplication by an integer is favorable at
`0.68`, while fixed division and reciprocal operations are slower:

| Operation | Fixed/base |
| --- | ---: |
| `reciprocal by int` | `1.34` |
| `reciprocal -> fixed` | `2.02` |
| `divide by int` | `1.99` |
| `divide` | `1.48` |
| `mul reciprocal` | `1.19` |

Direct conversion back to float or double is approximately neutral. The
floor, ceil, and round conversions to float or double favor the fixed path
because they reuse the integer rounding helpers.

## Reciprocal reuse

The measured fixed costs are:

- reciprocal construction: `1.74 ns/op`
- direct division: `1.44 ns/op`
- multiplication by a precomputed reciprocal: `0.74 ns/op`

Using the measured values, reciprocal reuse wins when:

```text
1.74 + 0.74 * N < 1.44 * N
```

The break-even point is therefore three uses of the same reciprocal in this
run. This supports precomputation for scanlines, sprites, spans, and other
workloads that reuse a divisor many times, but not for a one-off quotient.

## DDA line workload

The DDA loop computes its step once, repeatedly adds it, and rounds each point
to integer pixel coordinates. The fixed/float ratio remains between `0.41` and
`0.44` across all line lengths, corresponding to roughly a `2.3x` to `2.4x`
advantage for the fixed arithmetic path.

The stable ratio across increasing lengths indicates that the result reflects
the steady-state stepping loop rather than only setup overhead.

## Sprite scaler workload

The sprite scaler precomputes source-coordinate steps, repeatedly accumulates
them, and floors each source coordinate to an integer sample index.

The fixed/float ratio is between `0.05` and `0.07`, or roughly a `14x` to `20x`
advantage for the arithmetic-only fixed path in this run. The benchmark does
not access source or destination pixels, so this ratio must not be interpreted
as an expected whole-renderer speedup. Memory traffic, clipping, format
conversion, and blending can dominate a real blitter.

## Rotoscaler workload

The rotoscaler adds fixed-point multiply during setup, computes affine X and Y
step vectors, advances two coordinates per pixel, and floors them to source
indices.

The fixed/float ratio remains between `0.28` and `0.29`, corresponding to about
a `3.4x` to `3.6x` arithmetic-only advantage. The advantage is smaller than the
axis-aligned scaler because affine setup includes more multiplication, but the
repeated-addition and floor-heavy inner loop still favors fixed point.

## Interpretation

This run supports a hybrid use of the library. Float remains the simpler choice
for broad transform, animation, camera, and setup code. Fixed point is most
attractive after final step vectors and starting coordinates have been
computed, when a hot loop repeatedly adds values and converts them to integer
sample positions.

The useful fixed-point targets are:

- DDA, edge, and span walkers
- nearest-neighbor source-coordinate stepping
- affine and rotated sprite stepping
- subpixel accumulators with integer-facing output

The less favorable targets are:

- one-off fixed division or reciprocal construction
- multiply-heavy general-purpose math
- code that repeatedly crosses between float and fixed representations

These measurements are specific to this compiler, processor, optimization
level, and arithmetic-only benchmark structure. They should be rerun on each
target platform before making an engine-wide representation decision.

## Final considerations

Modern CPUs have fast floating-point pipelines, wide SIMD instruction sets,
and compilers that optimize float arithmetic aggressively. Fixed point should
therefore not be justified by the old assumption that integer arithmetic is
always faster than floating point. The scalar results in this report directly
contradict that assumption for multiply, divide, and reciprocal operations.

Fixed point remains relevant because some game-engine workloads are not merely
performing general arithmetic. Rasterizers, sprite scalers, span walkers, tile
samplers, and subpixel accumulators repeatedly advance coordinates and then
convert them into integer indices. In those loops, a fixed-point representation
keeps the value in the same integer domain as the final output. Addition,
masking, and extraction of the whole part can replace repeated floating-point
rounding or conversion work. The DDA, scaler, and rotoscaler measurements show
that this structural advantage still matters on the contemporary CPU used for
this report.

Small per-iteration savings also matter when multiplied by the number of
pixels, particles, collision candidates, or simulation entities processed in a
frame. Reducing a hot loop by even a few instructions can improve frame-time
headroom, leave more CPU time for gameplay and rendering, or make performance
less sensitive to large bursts of work. The arithmetic-only workload ratios
are not whole-engine speedup predictions, but they identify loops where further
integration testing is justified.

Fixed point can also provide an explicit quantization model. Positions, rates,
and accumulated values have a known resolution, and the rounding policy is
visible in the API. This can be useful for deterministic gameplay, replay,
lockstep synchronization, rollback state, and tests that require identical raw
results. That benefit is independent of whether floating-point arithmetic is
fast. It comes from controlling representation and rounding rather than from
instruction throughput.

Determinism is not automatic, however. Inputs must remain within the unchecked
range of the library, overflow must be avoided, and portable configurations
must be selected when code runs across different C implementations. In
particular, the optional signed-shift paths deliberately rely on behavior that
is implementation-defined or restricted to nonnegative inputs. Code that needs
cross-platform reproducibility should use the defined default multiply and
division paths and the portable floor/ceil path where arithmetic signed shifts
cannot be assumed.

The practical conclusion is not to replace all engine math with fixed point.
Float remains appropriate for transforms, cameras, animation, matrices,
trigonometry, and other broad or multiply-heavy calculations. Fixed point is
most valuable as a targeted representation for bounded, integer-facing,
addition-heavy systems. A hybrid engine can perform high-level setup in float,
convert stable starting values and increments once, and use fixed point only in
the hot or deterministic portion of the pipeline.
