# fpmath benchmark details

This document explains exactly what the benchmark suite measures, how the inputs are built, and what each reported row means.

## Goal of the benchmark

The benchmark is meant to compare:

- a signed 32-bit fixed-point representation (`fix32_t`)
- a floating-point baseline

for two kinds of work:

1. scalar arithmetic and conversion primitives
2. graphics-style workloads built mostly from those primitives

The benchmark is intentionally arithmetic-focused. The DDA, sprite, and rotoscaler tests do **not** draw to memory; they only execute the math and feed the results into checksum sinks so the compiler cannot remove the work.

## How to run it

```sh
make benchmark
./build/benchmark
```

Optional arguments:

```sh
./build/benchmark [sample-count] [repeat-count]
```

To build or run the signed-shift multiplication and division variants:

```sh
make benchmark-shift-mul
make run-benchmark-shift-mul BENCH_ARGS="8192 100"
make benchmark-shift-div
make run-benchmark-shift-div BENCH_ARGS="8192 100"
```

Defaults:

- `sample-count = 16384`
- `repeat-count = 2000`

## Measurement method

The benchmark uses `clock()` and reports CPU time.

There are four timing helpers:

- `run_benchmark()` for scalar rows
- `run_line_benchmark()` for the DDA line cases
- `run_sprite_benchmark()` for the sprite scaler cases
- `run_rotoscale_benchmark()` for the rotoscaler cases

The output is normalized like this:

- scalar section: **ns/op**
- DDA line section: **ns/pixel**
- sprite scaler section: **ns/pixel**
- rotoscaler section: **ns/pixel**

Each table also prints a `fixed/float` ratio:

- `< 1.0`: fixed-point is faster
- `> 1.0`: floating point is faster

## Important benchmark configuration choices

### Fixed inputs use explicit rounded conversion

The benchmark uses `fix32_round_from_float()` when preparing fixed-point inputs and when measuring conversion to a fixed-point result. This preserves the benchmark's original nearest-value conversion semantics now that `fix32_from_float()` is the explicit truncating constructor.

### The benchmark prevents dead-code elimination

The shared benchmark module defines:

- `volatile int64_t g_int_sink`
- `volatile double g_float_sink`

Each benchmark accumulates a checksum and stores it into one of these sinks at the end.

The DDA, sprite, and rotoscaler tests also fold their computed coordinates into a running checksum rather than discarding them.

## Input generation

All benchmark inputs are generated once in `benchmark_data_init()` and reused across all timed runs.

### Pseudorandom generator

The shared benchmark module uses a simple linear congruential generator:

```c
state = state * 1664525u + 1013904223u;
```

with a fixed seed:

```c
0x9e3779b9u
```

This keeps runs deterministic for the same build and parameters.

### Generated arrays

| Array | Type | Range / meaning |
| --- | --- | --- |
| `int_inputs` | `int32_t[]` | integers in `[-30000, 30000]` |
| `small_int_inputs` | `int32_t[]` | integer multipliers in `[-8, 8]` |
| `int_div_inputs` | `int32_t[]` | nonzero integer divisors in `[-16, -1] U [1, 16]` |
| `float_inputs` | `float[]` | floats in `[-256.0, 256.0]` |
| `double_inputs` | `double[]` | double versions of `float_inputs` |
| `sum_inputs` | `float[]` | floats in `[-0.5, 0.5]` |
| `div_inputs` | `float[]` | nonzero divisors roughly in `[-16.5, -0.5] U [0.5, 16.5]` |
| `float_reciprocals` | `float[]` | `1.0f / div_inputs[i]` |
| `fixed_inputs` | `fix32_t[]` | `fix32_round_from_float(float_inputs[i])` |
| `sum_fixed_inputs` | `fix32_t[]` | `fix32_round_from_float(sum_inputs[i])` |
| `fixed_div_inputs` | `fix32_t[]` | `fix32_round_from_float(div_inputs[i])` |
| `fixed_reciprocals` | `fix32_t[]` | `fix32_reciprocal(fixed_div_inputs[i])` |

`div_inputs` deliberately avoids zero so the division and reciprocal rows do not benchmark divide-by-zero handling.

## Scalar benchmark section

The scalar section prints:

```text
operation | fixed ns/op | base ns/op | fixed/base
```

The number of operations for each row is:

```text
sample_count * repeat_count
```

### Scalar rows

| Row label | Fixed-point path | Baseline path | Notes |
| --- | --- | --- | --- |
| `int -> representation` | `fix32_from_int(int_inputs[i])` | `(float)int_inputs[i]` | Integer conversion |
| `float -> fixed trunc` | `fix32_from_float(float_inputs[i])` | `float_inputs[i]` | Truncating float conversion |
| `float -> fixed round` | `fix32_round_from_float(float_inputs[i])` | `float_inputs[i]` | Rounded float conversion |
| `double -> fixed trunc` | `fix32_from_double(double_inputs[i])` | `double_inputs[i]` | Truncating double conversion |
| `double -> fixed round` | `fix32_round_from_double(double_inputs[i])` | `double_inputs[i]` | Rounded double conversion |
| `sum` | repeated `fix32_add()` | repeated float addition | Accumulation throughput |
| `subtract` | `fix32_sub()` | float subtraction | Subtraction throughput |
| `multiply` | `fix32_mul()` | float multiplication | Fixed-by-fixed multiplication |
| `multiply by int` | `fix32_mul_by_int()` | float-by-int multiplication | Integer scaling |
| `reciprocal by int` | `fix32_reciprocal_by_int()` | float reciprocal converted to fixed | Integer reciprocal |
| `reciprocal -> fixed` | `fix32_reciprocal()` | float reciprocal converted to fixed | Fixed reciprocal |
| `divide by int` | `fix32_div_by_int()` | float-by-int division | Integer division |
| `divide` | `fix32_div()` | float division | Uses nonnegative numerators so the signed-shift variant stays within its documented domain |
| `mul reciprocal` | `fix32_mul()` with a precomputed reciprocal | float multiplication with a reciprocal | Reciprocal reuse |
| `trunc -> int` | `fix32_trunc_to_int()` | float cast to integer | Truncation toward zero |
| `ceil -> int` | `fix32_ceil_to_int()` | local float ceil helper | Integer ceiling |
| `floor -> int` | `fix32_floor_to_int()` | local float floor helper | Integer floor |
| `round -> int` | `fix32_round_to_int()` | local float round helper | Half-away-from-zero rounding |
| `floor -> fixed` | `fix32_floor()` | float floor helper | Result remains in its native representation |
| `ceil -> fixed` | `fix32_ceil()` | float ceil helper | Result remains in its native representation |
| `round -> fixed` | `fix32_round()` | float round helper | Result remains in its native representation |
| `value -> float` | `fix32_to_float()` | float identity | Exact value conversion |
| `ceil -> float` | `fix32_ceil_to_float()` | float ceil helper | Whole-number float result |
| `floor -> float` | `fix32_floor_to_float()` | float floor helper | Whole-number float result |
| `round -> float` | `fix32_round_to_float()` | float round helper | Whole-number float result |
| `value -> double` | `fix32_to_double()` | double identity | Exact value conversion |
| `ceil -> double` | `fix32_ceil_to_double()` | local double ceil helper | Whole-number double result |
| `floor -> double` | `fix32_floor_to_double()` | local double floor helper | Whole-number double result |
| `round -> double` | `fix32_round_to_double()` | local double round helper | Whole-number double result |

### Reciprocal benchmark detail

The `reciprocal -> fixed` row deserves special attention.

Fixed-point side:

```c
fix32_reciprocal(x)
```

Floating-point side:

```c
fix32_round_from_float(1.0f / x)
```

The reason for converting the float reciprocal back to fixed is that this row is meant to compare **reciprocal computation as a way to obtain a fixed-point reciprocal value**, not merely a raw float reciprocal.

One subtlety:

- `fix32_reciprocal()` is based on `fix32_div()`
- `fix32_div()` inherits truncation from integer division
- `fix32_round_from_float()` rounds to nearest using the library's explicit conversion rule

So for reciprocals that are not exactly representable, the two paths can differ by **one raw least-significant bit**. That is expected and reflects the different rounding/truncation behavior.

### Why there are both `divide` and `mul reciprocal`

They answer different questions:

- `divide`: cost of doing the quotient directly each time
- `mul reciprocal`: cost of using a reciprocal that was already precomputed and reused

This is useful because a reciprocal path only makes sense as an optimization when the divisor is reused enough times to amortize the reciprocal computation.

## DDA line benchmark section

The DDA section prints:

```text
line length | pixels | fixed ns/pixel | float ns/pixel | fixed/float
```

### Purpose

This is a virtual line-drawing workload. It exercises:

- one-time step calculation
- repeated addition
- repeated rounding to integer coordinates

without including framebuffer writes.

### Endpoint construction

For a requested `line_length`, the code builds:

```c
dx = line_length;
dy = (line_length * 5) / 8 + 1;

start_x = -(line_length / 3) - 7;
start_y = -(line_length / 4) - 5;
end_x = start_x + dx;
end_y = start_y + dy;
```

### DDA step count

The number of DDA steps is:

```c
max(abs(dx), abs(dy))
```

The reported pixel count is:

```c
steps + 1
```

### Fixed-point DDA path

The fixed-point line routine:

1. converts the start coordinates to fixed
2. computes:
   - `step_x = fix32_div_by_int(fix32_from_int(dx), steps)`
   - `step_y = fix32_div_by_int(fix32_from_int(dy), steps)`
3. advances `x` and `y` by repeated `fix32_add()`
4. converts each point to pixel coordinates with `fix32_round_to_int()`
5. folds the pixel into the checksum:

```c
pixel_y * DDA_CANVAS_STRIDE + pixel_x
```

where:

```c
DDA_CANVAS_STRIDE = 4099
```

### Float DDA path

The float line routine mirrors the same logic:

1. float start coordinates
2. float `step_x` and `step_y`
3. repeated float addition
4. rounding via `float_round_to_int()`
5. the same checksum structure

### Tested line lengths

The code benchmarks these lengths:

- `32`
- `128`
- `512`
- `2048`
- `8192`

The reported `ns/pixel` value is:

```text
seconds * 1e9 / (point_count * repeat_count)
```

## Sprite scaler benchmark section

The sprite section prints:

```text
case | dest pixels | fixed ns/pixel | float ns/pixel | fixed/float
```

### Purpose

This is a virtual nearest-neighbor style coordinate-generation benchmark. It exercises:

- source-step setup
- repeated coordinate accumulation
- floor conversion for source sampling

again without touching actual image memory.

### Fixed-point sprite path

For each case, the fixed-point scaler computes:

```c
step_x = fix32_div_by_int(fix32_from_int(src_width), dest_width);
step_y = fix32_div_by_int(fix32_from_int(src_height), dest_height);
```

Then for each destination pixel:

1. `source_y` is advanced once per output row
2. `source_x` is advanced once per output column
3. sampled source indices are produced with:
   - `fix32_floor_to_int(source_y)`
   - `fix32_floor_to_int(source_x)`
4. the sampled coordinate is folded into the checksum:

```c
sample_y * src_width + sample_x
```

### Float sprite path

The float path mirrors that logic:

```c
step_x = (float)src_width / (float)dest_width;
step_y = (float)src_height / (float)dest_height;
```

and uses `float_floor_to_int()` for sampling.

### Tested sprite cases

| Label | Source | Destination |
| --- | --- | --- |
| `16x16 -> 24x24 (1.5x)` | `16x16` | `24x24` |
| `32x24 -> 64x42 (2.0x/1.75x)` | `32x24` | `64x42` |
| `64x48 -> 160x96 (2.5x/2.0x)` | `64x48` | `160x96` |
| `96x64 -> 288x160 (3.0x/2.5x)` | `96x64` | `288x160` |
| `128x96 -> 512x384 (4.0x)` | `128x96` | `512x384` |
| `160x128 -> 1024x768 (6.4x)` | `160x128` | `1024x768` |
| `256x192 -> 2048x1536 (8.0x)` | `256x192` | `2048x1536` |
| `512x384 -> 4096x3072 (8.0x)` | `512x384` | `4096x3072` |
| `1024x768 -> 8192x6144 (8.0x)` | `1024x768` | `8192x6144` |

The reported `ns/pixel` value is:

```text
seconds * 1e9 / (dest_width * dest_height * repeat_count)
```

## Rotoscaler benchmark section

The rotoscaler section prints:

```text
case | dest pixels | fixed ns/pixel | float ns/pixel | fixed/float
```

### Purpose

This is a virtual rotated-sprite coordinate-generation benchmark. It is intended to model the arithmetic core of a software rotoscaler:

- one-time setup of rotated/scaled step vectors
- repeated per-row start updates
- repeated per-pixel source-coordinate stepping
- floor conversion to source texel indices

Like the sprite scaler test, it does not fetch or write pixels.

### Fixed-point rotoscaler path

For each case, the fixed-point routine computes:

- X-axis source step after scale and rotation
- Y-axis source step after scale and rotation
- a rotated/scaled origin in source space for the top-left destination pixel

The main setup is:

```c
scale_x = fix32_div_by_int(fix32_from_int(src_width), dest_width);
scale_y = fix32_div_by_int(fix32_from_int(src_height), dest_height);

step_u_x = fix32_mul(cos_angle, scale_x);
step_v_x = -fix32_mul(sin_angle, scale_x);
step_u_y = fix32_mul(sin_angle, scale_y);
step_v_y = fix32_mul(cos_angle, scale_y);
```

Then each output row:

1. starts from a row origin `(row_u, row_v)`
2. advances `(u, v)` across the row by repeated addition
3. converts to source texel indices with `fix32_floor_to_int()`
4. folds the sampled coordinate into the checksum
5. advances the next row origin by `(step_u_y, step_v_y)`

### Float rotoscaler path

The float routine mirrors the same structure:

- float `scale_x`, `scale_y`
- float `step_u_x`, `step_v_x`, `step_u_y`, `step_v_y`
- float row origins and repeated per-pixel additions
- sampling via `float_floor_to_int()`

### Why this benchmark is useful

The sprite scaler benchmark covers axis-aligned scaling. The rotoscaler adds:

- rotated step vectors
- more setup multiplies
- non-axis-aligned source traversal

This makes it a closer match for:

- software sprite rotation
- affine blits
- camera-space billboard sampling
- classic 2D rotoscaling effects

### Tested rotoscaler cases

| Label | Source | Destination | Angle |
| --- | --- | --- | --- |
| `32x32 -> 64x64 @ 15 deg` | `32x32` | `64x64` | `15°` |
| `64x48 -> 160x120 @ 30 deg` | `64x48` | `160x120` | `30°` |
| `96x64 -> 288x192 @ 45 deg` | `96x64` | `288x192` | `45°` |
| `128x96 -> 512x384 @ 22.5 deg` | `128x96` | `512x384` | `22.5°` |
| `256x192 -> 1024x768 @ 60 deg` | `256x192` | `1024x768` | `60°` |

The reported `ns/pixel` value is:

```text
seconds * 1e9 / (dest_width * dest_height * repeat_count)
```

## How to read the benchmark as a design tool

### Use the scalar section to answer primitive-cost questions

Examples:

- Is `fix32_round_from_float()` more expensive than leaving a value as float?
- How expensive is `fix32_mul()` under the current 32-bit or 64-bit multiply configuration?
- Is direct divide or reciprocal reuse a better fit for the intended workload?

### Use the DDA section to answer incremental-rasterization questions

Examples:

- Does fixed point pay off when the hot loop is mostly addition plus integer rounding?
- Is setup cost still visible for short lines?
- Does the steady-state cost stabilize as the line gets longer?

### Use the sprite section to answer repeated-resampling questions

Examples:

- How expensive is floor-based coordinate generation at large image sizes?
- Does fixed point stay consistent as the output size grows?
- Is the arithmetic cost likely to matter before memory traffic dominates in a real renderer?

### Use the rotoscaler section to answer affine-blit questions

Examples:

- How much does fixed point help once rotation is added to scaling?
- Does the advantage of fixed point survive the extra setup multiplies needed for rotated step vectors?
- Is a fixed-point affine blitter likely to benefit even when the source walk is no longer axis aligned?

## What the benchmark does not measure

The benchmark does **not** measure:

- actual pixel writes
- texture fetches
- cache behavior from real framebuffers or sprite data
- branch-heavy clipping logic
- alpha blending, compositing, or color conversion
- end-to-end renderer throughput

So the results should be interpreted as:

- arithmetic cost for the chosen representation
- not a full rendering-system performance claim

## Summary

The benchmark suite is structured to answer four separate performance questions:

1. What is the isolated cost of the fixed-point primitives?
2. How do fixed and float behave in an incremental line-stepping workload?
3. How do fixed and float behave in repeated scaling/sampling coordinate generation?
4. How do fixed and float behave in rotated scaling / affine source-coordinate generation?

The scalar rows are useful for primitive design decisions. The DDA, sprite, and rotoscaler sections are useful for deciding whether those primitive costs matter in graphics-style code that mostly advances coordinates and converts them back to integer sample positions.
