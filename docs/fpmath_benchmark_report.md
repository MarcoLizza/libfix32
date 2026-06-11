# fpmath benchmark report

This report captures a benchmark run of `benchmark.c` after adding scalar, DDA, sprite-scaler, and rotoscaler workloads. The analysis is written with a game engine in mind, especially the blitting system and affine sprite pipeline.

## Run configuration

Build:

```sh
cc -std=c99 -O3 -Wall -Wextra -pedantic src/tests/benchmark.c -o benchmark
```

Execution:

```sh
./benchmark 8192 100
```

Parameters:

- `sample-count = 8192`
- `repeat-count = 100`

Rationale:

- `8192 * 100 = 819200` scalar operations per row gives reasonably stable scalar timing.
- `repeat-count = 100` is still practical for the larger sprite and rotoscaler cases.
- The larger workload sizes expose steady-state behavior rather than only setup overhead.

Benchmark mode:

- `benchmark.c` disables `FIX32_ENABLE_DEBUG_CHECKS` before including the header.
- These numbers therefore describe arithmetic behavior, not debug-assert overhead.

## Raw output

```text
scalar benchmarks
samples=8192 repeats=100 operations-per-case=819200
operation                   fixed ns/op    float ns/op  fixed/float
int -> representation              0.72           1.25         0.57
float -> representation            4.10           0.51         7.98
sum                                0.26           0.52         0.50
multiply                           0.67           0.52         1.29
reciprocal -> fixed                1.17           1.11         1.05
divide                             1.37           1.14         1.21
mul reciprocal                     0.56           0.44         1.29
ceil -> int                        0.40           0.76         0.53
floor -> int                       0.25           0.76         0.33
round -> int                       0.90           0.87         1.03
value -> float                     0.44           0.44         1.00
ceil -> float                      0.44           1.24         0.35
floor -> float                     0.44           1.22         0.36
round -> float                     0.97           0.90         1.08

DDA line benchmarks
repeats-per-case=100
line length        pixels fixed ns/pixel float ns/pixel  fixed/float
32                     33           1.21           1.82         0.67
128                   129           1.16           1.78         0.65
512                   513           1.17           1.83         0.64
2048                 2049           1.15           1.80         0.64
8192                 8193           1.15           1.78         0.65

sprite scaler benchmarks
repeats-per-case=100
case                              dest pixels fixed ns/pixel float ns/pixel  fixed/float
16x16 -> 24x24 (1.5x)                     576           0.17           0.87         0.20
32x24 -> 64x42 (2.0x/1.75x)              2688           0.16           0.83         0.20
64x48 -> 160x96 (2.5x/2.0x)             15360           0.16           0.86         0.18
96x64 -> 288x160 (3.0x/2.5x)            46080           0.15           0.84         0.18
128x96 -> 512x384 (4.0x)               196608           0.15           0.93         0.17
160x128 -> 1024x768 (6.4x)             786432           0.14           0.69         0.20
256x192 -> 2048x1536 (8.0x)           3145728           0.14           0.71         0.20
512x384 -> 4096x3072 (8.0x)          12582912           0.14           0.67         0.21
1024x768 -> 8192x6144 (8.0x)         50331648           0.14           0.68         0.20

rotoscaler benchmarks
repeats-per-case=100
case                                  dest pixels fixed ns/pixel float ns/pixel  fixed/float
32x32 -> 64x64 @ 15 deg                      4096           0.55           1.17         0.47
64x48 -> 160x120 @ 30 deg                   19200           0.51           1.15         0.45
96x64 -> 288x192 @ 45 deg                   55296           0.49           1.14         0.43
128x96 -> 512x384 @ 22.5 deg               196608           0.49           1.14         0.43
256x192 -> 1024x768 @ 60 deg               786432           0.48           1.14         0.42
```

## Executive summary

For this run:

- **fixed-point is a strong fit for hot blit-style coordinate stepping**
- **floating point is still the better general-purpose representation for float-heavy setup and one-off arithmetic**
- **axis-aligned scaling is the strongest fixed-point win**
- **rotation reduces the fixed-point advantage, but fixed still wins clearly in the affine inner loop**

The strongest fixed-point wins are in:

- repeated addition (`sum`)
- `floor -> int`
- `ceil -> int`
- DDA stepping
- sprite-scaler coordinate generation
- rotoscaler coordinate generation

The strongest floating-point wins are in:

- `float -> representation`
- general scalar multiply/divide style work
- one-off reciprocal generation

Overall recommendation:

- use **float** for broad engine-side math and transform setup
- use **fixed-point** inside the hot blit/scaler/affine stepping loops

## Scalar analysis

### Where fixed-point wins

| Operation | Observation | Interpretation |
| --- | --- | --- |
| `int -> representation` | fixed is about `1.75x` faster | Cheap when positions or sizes start as integers |
| `sum` | fixed is about `2.0x` faster | Confirms fixed-point is well-suited to incremental stepping |
| `ceil -> int` | fixed is about `1.9x` faster | Useful for endpoints, bounds, and raster limits |
| `floor -> int` | fixed is about `3.0x` faster | Very relevant for texel/sample lookup |
| `ceil -> float` | fixed is about `2.9x` faster | Quantization back to whole-number float is cheap |
| `floor -> float` | fixed is about `2.8x` faster | Same general story |

### Where float wins

| Operation | Observation | Interpretation |
| --- | --- | --- |
| `float -> representation` | fixed is about `8.0x` slower | Per-pixel float-to-fixed conversion is a bad idea |
| `multiply` | fixed is about `1.3x` slower | Safe 16:16 multiply is not a scalar win here |
| `reciprocal -> fixed` | fixed is slightly slower | One-off reciprocal generation is not a fixed-point strength |
| `divide` | fixed is about `1.2x` slower | Direct divide is slightly more expensive |
| `mul reciprocal` | fixed is about `1.3x` slower | Reusing a reciprocal helps structure, but not scalar cost by itself |

### Mostly neutral cases

| Operation | Observation | Interpretation |
| --- | --- | --- |
| `round -> int` | almost equal, slight float win | Not a major architectural separator |
| `value -> float` | effectively equal | Converting back to float is not a dominant concern |
| `round -> float` | almost equal, slight float win | Also not a deciding factor |

### What this means

The scalar results alone do **not** justify using fixed-point everywhere. In fact, if the engine stays mostly in floating point and repeatedly converts into fixed, the conversion penalty will dominate.

The scalar data says:

- fixed-point is excellent for **addition-heavy, integer-facing** arithmetic
- float is better for **general-purpose scalar math**

That is exactly why fixed-point makes more sense in the blitter inner loop than in the entire engine.

## Divide and reciprocal interpretation

Measured fixed costs:

- `reciprocal -> fixed`: `1.17 ns/op`
- `divide`: `1.37 ns/op`
- `mul reciprocal`: `0.56 ns/op`

This implies:

- direct divide is better for one-off quotients
- reciprocal precomputation becomes attractive when the reciprocal is reused

A rough break-even estimate for the fixed path is:

```text
1.17 + 0.56 * N < 1.37 * N
```

which starts paying off after about **2 uses**.

For a blitter, that is a favorable result because:

- source X/Y step values are reused for entire scanlines or sprites
- affine step vectors are reused for many pixels
- per-sprite scale factors are typically reused hundreds or thousands of times

So the practical recommendation remains:

- **do not** replace a one-off divide with reciprocal multiply
- **do** precompute reciprocals or derived step values when the divisor is reused in the hot loop

## DDA line analysis

The DDA benchmark represents classic incremental rasterization:

- compute step once
- repeatedly add
- round to integer pixel coordinates

### Result

Fixed-point wins consistently:

| Line length | fixed/float |
| --- | --- |
| `32` | `0.67` |
| `128` | `0.65` |
| `512` | `0.64` |
| `2048` | `0.64` |
| `8192` | `0.65` |

That is a stable advantage of about **1.5x to 1.56x** in favor of fixed-point.

### Interpretation

This is strong evidence that fixed-point is a good fit for:

- line drawing
- scan conversion
- span/edge walkers
- subpixel accumulators
- any blit/raster path that mostly advances by repeated addition

The performance stays stable as the line gets longer, which means the win is not just a tiny-case artifact.

## Sprite scaler analysis

This section is the clearest match for an axis-aligned blitter or nearest-neighbor software scaler.

The workload models:

- precomputed per-axis source step values
- repeated source-coordinate accumulation
- floor conversion to texel indices

### Result

Fixed-point wins very strongly for every case:

| Case | fixed/float |
| --- | --- |
| `16x16 -> 24x24` | `0.20` |
| `32x24 -> 64x42` | `0.20` |
| `64x48 -> 160x96` | `0.18` |
| `96x64 -> 288x160` | `0.18` |
| `128x96 -> 512x384` | `0.17` |
| `160x128 -> 1024x768` | `0.20` |
| `256x192 -> 2048x1536` | `0.20` |
| `512x384 -> 4096x3072` | `0.21` |
| `1024x768 -> 8192x6144` | `0.20` |

That is roughly a **4.8x to 5.9x speedup** in the arithmetic-only portion of the scaler.

### Why this matters for a blitter

A nearest-neighbor or point-sampled blit/scaler often does exactly this:

1. compute X/Y source steps once
2. accumulate source X/Y across pixels
3. floor or truncate to integer source indices
4. fetch/copy/blend

This benchmark isolates steps 1-3, and fixed-point dominates them.

### Caveat

A real blitter also pays for:

- texture/source reads
- destination writes
- format conversion
- alpha/blend logic
- clipping
- cache effects

So the whole renderer will not be `5x` faster just because the arithmetic is. But for CPU blitters, source-coordinate generation is absolutely hot-loop work, so a large arithmetic win is meaningful.

## Rotoscaler analysis

This is the new workload most relevant to affine blitting and rotated sprites.

It models:

- setup of rotated/scaled X and Y step vectors
- per-row source-origin updates
- per-pixel source-coordinate stepping
- floor conversion to integer source texel indices

### Result

Fixed-point still wins across every case:

| Case | fixed/float |
| --- | --- |
| `32x32 -> 64x64 @ 15 deg` | `0.47` |
| `64x48 -> 160x120 @ 30 deg` | `0.45` |
| `96x64 -> 288x192 @ 45 deg` | `0.43` |
| `128x96 -> 512x384 @ 22.5 deg` | `0.43` |
| `256x192 -> 1024x768 @ 60 deg` | `0.42` |

That is about a **2.1x to 2.4x speedup** for the arithmetic-only portion of the rotoscaler.

### Why the win is smaller than the plain sprite scaler

The sprite scaler mainly benefits from:

- additions
- floor conversion
- simple per-axis stepping

The rotoscaler adds extra setup work:

- rotated X/Y step-vector construction
- more multiplies in setup
- less trivial source-coordinate geometry

Since scalar multiply is not a fixed-point win in this configuration, the fixed-point advantage shrinks relative to the axis-aligned scaler. Even so, the inner loop still benefits from fixed-point stepping enough to keep fixed clearly ahead overall.

### What this means for affine blitters

This result is important:

- fixed-point is not only good for straight scaling
- it still looks favorable once rotation is added

So for a software engine doing:

- rotated sprite draws
- affine texture mapping on spans
- classic 2D rotoscaling effects

fixed-point remains a strong candidate for the hot coordinate-generation path.

## Pros and cons by representation

## Fixed-point pros

- Excellent for repeated-addition stepping
- Strong floor/ceil-to-int behavior
- Very strong fit for axis-aligned blit/scaler inner loops
- Still clearly beneficial for affine/rotated coordinate stepping
- Good deterministic integer-facing behavior
- Natural match for scanline, span, and source-coordinate accumulators

## Fixed-point cons

- Expensive to convert from float repeatedly
- General scalar multiply/divide/reciprocal are not a major win here
- Needs explicit range and precision discipline
- May require 64-bit intermediates or carefully constrained input ranges
- More specialized than float for broad engine-wide math

## Floating-point pros

- Best when values are already float
- Better scalar multiply/divide profile in this benchmark
- Better fit for general engine transforms, animation math, and one-off setup arithmetic
- Easier to compose across the whole engine
- Simpler for camera, matrix, angle, and transform code

## Floating-point cons

- Weaker for floor-heavy sample-index generation
- Weaker for repeated addition in raster-style loops
- Significantly weaker for the axis-aligned scaler workload
- Still clearly behind on the rotoscaler workload in this run

## Practical recommendation for a game engine blitting system

The data supports a **hybrid architecture**.

### Use float for:

- high-level transforms
- camera space
- animation and gameplay math
- rotation setup
- one-off scale and reciprocal calculations
- any code path where values already live naturally in float

### Use fixed-point for:

- per-pixel source-coordinate stepping
- scanline/span walkers
- nearest-neighbor sprite scaling
- affine/rotated sprite stepping
- floor-heavy source-index generation
- subpixel accumulators in hot blit loops

### Recommended pipeline

For a software blitter or affine sprite drawer:

1. compute high-level transform parameters in float if convenient
2. derive final per-axis or affine step values once per sprite/span
3. convert those step values and starting coordinates once into fixed
4. run the hot inner loop in fixed-point
5. derive texel indices with floor/truncate conversion

This matches the benchmark well:

- fixed loses badly when asked to absorb float conversion over and over
- fixed wins strongly when it stays in fixed for the hot loop

## Bottom line

If the question is:

> “Should a game engine use fixed-point everywhere?”

the answer from this benchmark is **no**.

If the question is:

> “Should a software blitting system strongly consider fixed-point for its hot coordinate-generation loops?”

the answer is **yes**.

And with the new rotoscaler result, that answer now extends beyond plain scaling:

- **axis-aligned scaling:** fixed-point looks extremely strong
- **rotated / affine scaling:** fixed-point still looks clearly favorable

So the best use of this library in a game engine is not as a universal math type, but as a focused tool for the inner arithmetic of the blitting and raster pipeline.
