# Benchmarks (report)

This report captures the current default benchmark suite, including all scalar
API benchmarks and the DDA, sprite-scaler, and rotoscaler workloads. The
workload results measure coordinate arithmetic only; they do not include
framebuffer access, texture sampling, blending, clipping, or other renderer
costs.

## Run configuration

The measurements were recorded on June 27, 2026 with:

- fix32 0.3.0
- AMD Ryzen 7 8845HS
- Ubuntu x86-64, Linux 6.17.0-35-generic
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

The fixed and floating-point rounding helpers are both inline-visible to the
compiler; the benchmark build enables `FIX32_STATIC_INLINE` for the fixed-point
header. After conversion to integer sample coordinates, the DDA, scaler, and
rotoscaler paths also use the same `int64_t` checksum operation.

## Raw output

```text
scalar benchmarks
samples=8192 repeats=100 operations-per-case=819200
operation                   fixed ns/op     base ns/op   fixed/base
int -> representation              0.41           0.63         0.65
rational -> fixed                  1.43           0.75         1.90
float -> fixed trunc               0.67           0.92         0.73
float -> fixed round               0.97           0.61         1.58
double -> fixed trunc              0.50           0.61         0.81
double -> fixed round              0.91           0.61         1.50
sum                                0.23           0.61         0.38
subtract                           0.36           0.61         0.58
multiply                           0.73           0.61         1.20
multiply by int                    0.43           0.64         0.67
reciprocal by int                  1.23           0.68         1.80
reciprocal -> fixed                1.44           0.91         1.58
divide by int                      1.30           0.63         2.06
divide                             1.45           0.68         2.13
mul reciprocal                     0.75           0.62         1.21
trunc -> int                       0.41           0.49         0.84
ceil -> int                        0.42           0.59         0.71
floor -> int                       0.35           0.60         0.58
round -> int                       0.70           0.85         0.83
floor -> fixed                     0.42           1.02         0.41
ceil -> fixed                      0.60           1.03         0.59
round -> fixed                     0.66           1.31         0.50
value -> float                     0.63           0.62         1.02
ceil -> float                      0.62           1.01         0.61
floor -> float                     0.63           1.01         0.63
round -> float                     0.70           1.27         0.55
value -> double                    0.62           0.62         0.99
ceil -> double                     0.62           0.88         0.70
floor -> double                    0.63           0.93         0.68
round -> double                    0.70           1.40         0.50

DDA line benchmarks
repeats-per-case=100
line length        pixels fixed ns/pixel float ns/pixel  fixed/float
32                     33           1.52           1.82         0.83
128                   129           1.24           1.78         0.70
512                   513           1.21           1.75         0.69
2048                 2049           1.19           1.75         0.68
8192                 8193           1.19           1.79         0.66

sprite scaler benchmarks
repeats-per-case=100
case                              dest pixels fixed ns/pixel float ns/pixel  fixed/float
16x16 -> 24x24 (1.5x)                     576           0.17           0.87         0.20
32x24 -> 64x42 (2.0x/1.75x)              2688           0.15           0.84         0.17
64x48 -> 160x96 (2.5x/2.0x)             15360           0.15           0.84         0.18
96x64 -> 288x160 (3.0x/2.5x)            46080           0.14           0.83         0.17
128x96 -> 512x384 (4.0x)               196608           0.14           0.83         0.17
160x128 -> 1024x768 (6.4x)             786432           0.14           0.83         0.17
256x192 -> 2048x1536 (8.0x)           3145728           0.14           0.83         0.17
512x384 -> 4096x3072 (8.0x)          12582912           0.15           0.83         0.18
1024x768 -> 8192x6144 (8.0x)         50331648           0.15           0.83         0.17

rotoscaler benchmarks (incremental inverse mapping)
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.75           1.34         0.56
64x48 -> 160x120 @ 30 deg                   19200           0.76           1.42         0.53
96x64 -> 288x192 @ 45 deg                   55296           0.76           1.33         0.57
128x96 -> 512x384 @ 22.5 deg               196608           0.78           1.34         0.58
256x192 -> 1024x768 @ 60 deg               786432           0.76           1.34         0.57

rotoscaler benchmarks (direct inverse mapping)
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.91           0.92         0.98
64x48 -> 160x120 @ 30 deg                   19200           0.90           0.92         0.98
96x64 -> 288x192 @ 45 deg                   55296           0.90           0.92         0.99
128x96 -> 512x384 @ 22.5 deg               196608           0.92           0.91         1.01
256x192 -> 1024x768 @ 60 deg               786432           0.92           0.90         1.02
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
| DDA `line length` rows | Run an arithmetic-only DDA line stepper for the requested major-axis length. The `pixels` column is the number of generated points. |
| Sprite scaler `case` rows | Run nearest-neighbor-style source coordinate generation from the source dimensions to the destination dimensions shown in the label. |
| Rotoscaler incremental `case` rows | Run affine inverse mapping by incrementally advancing source coordinates across each row and down each column. |
| Rotoscaler direct `case` rows | Run affine inverse mapping by recomputing the source coordinate from destination `x` and `y` for every pixel. |

## Scalar results

The strongest fixed-point scalar results are the operations built from integer
addition, masking, and integer-facing rounding. Repeated addition costs
`0.23 ns/op` against `0.61 ns/op` for the float baseline. Fixed-valued floor,
ceil, and round have ratios from `0.41` to `0.59`.

Truncating float and double construction is cheaper than the corresponding
baseline loop in this run, with ratios of `0.73` and `0.81` for truncating
construction. Explicit rounded construction is slower than the baseline, with
ratios of `1.58` for float and `1.50` for double. These
sub-nanosecond conversion rows are particularly sensitive to compiler
optimization and should not be treated as standalone architectural evidence.

The rational conversion row measures `fix32_from_rational()` against a float
division of the same integer numerator and denominator followed by truncating
float-to-fixed conversion. The integer rational path is `1.90` times the float
baseline in this run because 64-bit scaled integer division is more expensive
than this compiler's float division and conversion sequence. Its value is
semantic rather than throughput: it avoids the intermediate float rounding step
and directly computes the truncated fixed-point ratio.

Fixed multiplication is `1.20` times the float cost under the default
64-bit-product configuration. Multiplication by an integer is favorable at
`0.67`, while fixed division and reciprocal operations are slower:

| Operation | Fixed/base |
| --- | ---: |
| `reciprocal by int` | `1.80` |
| `reciprocal -> fixed` | `1.58` |
| `divide by int` | `2.06` |
| `divide` | `2.13` |
| `mul reciprocal` | `1.21` |

Direct conversion back to float or double is approximately neutral in this run.
The floor, ceil, and round conversions to float or double favor the fixed path
because they reuse the integer rounding helpers.

## Reciprocal reuse

The measured fixed costs are:

- reciprocal construction: `1.44 ns/op`
- direct division: `1.45 ns/op`
- multiplication by a precomputed reciprocal: `0.75 ns/op`

Using the measured values, reciprocal reuse wins when:

```text
1.44 + 0.75 * N < 1.45 * N
```

The break-even point is therefore three uses of the same reciprocal in this run.
This supports precomputation for scanlines, sprites, spans, and other
workloads that reuse a divisor many times, but not for a one-off quotient.

## DDA line workload

The DDA loop computes its step once, repeatedly adds it, and rounds each point
to integer pixel coordinates. Ratios range from `0.66` to `0.70` for the larger
cases, with more timing variation in the short case. The result still favors
fixed-point stepping, but the margin is smaller and less uniform than in the
scaler workloads.

## Sprite scaler workload

The sprite scaler precomputes source-coordinate steps, repeatedly accumulates
them, and floors each source coordinate to an integer sample index.

The fixed/float ratio is between `0.17` and `0.20`, or roughly a `5.0x` to `5.9x`
advantage for the arithmetic-only fixed path in this run. The benchmark does
not access source or destination pixels, so this ratio must not be interpreted
as an expected whole-renderer speedup. Memory traffic, clipping, format
conversion, and blending can dominate a real blitter.

### Why the axis-aligned loop is favorable

The inner loop maintains only `source_x`. Each pixel performs one source-X
floor conversion, one fixed or float addition, and the checksum update.
`sample_y` is constant for the row, so its conversion and row-index term can be
amortized or hoisted. The Y accumulator advances only once per output row.

For this GCC build, the fixed inner loop was vectorized with 16-byte vectors.
The float loop was not vectorized because the general floor-to-int correction
introduces control flow. This is a legitimate result for the tested source, but
it is compiler- and formulation-dependent. A production scaler that can prove
all source coordinates nonnegative could use truncating float conversion as
floor, which may reduce the gap and make vectorization easier.

The nearly flat per-pixel timings from 24-pixel to 8192-pixel rows show that
setup and row overhead are already small for these cases. The benchmark is
therefore mostly measuring the steady-state coordinate walker. The two step
calculations are performed once before all 100 repetitions, so even the
smallest scaler case amortizes setup over `57600` sampled pixels. A real
renderer that recalculates steps for every small draw will expose more setup
cost than this benchmark.

### Precision and coverage limits

With 16 fractional bits, the direct step division introduces less than one raw
unit of truncation per axis. Repeated addition makes that quantization error
accumulate with output width or height, although it remains small for the
tested dimensions. A coordinate near a texel boundary can still select a
different sample from the float path.

The benchmark does not compare the sampled coordinates for equality. It also
tests only positive-origin upscaling with nearest-neighbor-style floor
sampling. Downscaling, negative or clipped origins, mirrored steps, edge
handling, and bilinear filtering need separate correctness and performance
measurements.

## Rotoscaler workload

The rotoscaler now reports two loop shapes.

The incremental inverse-mapping path adds the affine step vectors across the
row and down the column. Its fixed/float ratio is about `0.53` to `0.58`,
which is still a clear arithmetic-only advantage for the fixed path.

The direct inverse-mapping path recomputes the source coordinate from the
destination `x` and `y` each pixel. Its fixed/float ratio is about `0.98` to
`1.02`, which is roughly parity on this compiler and build. The extra
per-pixel multiplications make this form more expensive than the incremental
walker, and the fixed advantage is effectively gone once the direct float path
is measured on the same terms.

### Why the rotoscaler gap changes

The incremental loop keeps `u` and `v` as running coordinates. That matches the
common affine-walker shape used in software blitters and avoids recomputing the
full transform for every pixel.

The direct loop trades recurrence for recomputation. It is simpler to reason
about, but it spends more work per pixel on multiply operations. In the current
build, the float direct path is roughly equal to or slightly faster than the
fixed direct path, so this is mostly an upper bound on the cost of explicit
affine recomputation rather than a fixed-point win.

The fixed setup performs angle conversion, step calculation, and origin
calculation with multiplication and division. However, the benchmark computes
that setup once before all 100 repetitions. It is heavily amortized and is not
the main explanation for the measured ratios. A real renderer repeats setup
once per draw, so very small sprites, short spans, or highly fragmented batches
will show a smaller advantage than these steady-state results.

### Precision and rendering implications

The rotoscaler quantizes sine, cosine, scale, four affine step components, and
the origin. Step error then accumulates across columns and rows in the
incremental form, while the direct form recomputes the same affine expression
for every pixel. Both give stable and deterministic sampling, but they can also
produce a different texel choice near boundaries, visible edge drift on long
spans, or asymmetric results for transforms that are mathematically equivalent.

The benchmark deliberately omits clipping, wrapping, clamping, source fetches,
destination writes, transparency, and interpolation. Rotated traversal also
has less predictable memory locality than axis-aligned scaling, so source-cache
behavior may dominate a real implementation and reduce the arithmetic
advantage. Bilinear filtering would add fractional-weight calculation and four
source reads, changing the operation balance substantially.

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
