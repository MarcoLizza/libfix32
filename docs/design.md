# fix32 design recap

This document summarizes the design choices and rationale behind the single-header fixed-point module in `fix32.h` and the associated benchmark suite in `src/bench`.

## Goals

- Keep the library in straight C99.
- Keep the library single-header, with either one `FIX32_IMPLEMENTATION` translation unit or opt-in `FIX32_STATIC_INLINE` helpers, and no external dependencies.
- Compare a signed 32-bit fixed-point representation against floating point for conversion-heavy and graphics-style workloads.
- Avoid math-library helpers such as `lroundf()` and `ceilf()` so the implementation stays self-contained.

## Representation

- The storage type is `int32_t` (`fix32_t`).
- The default format is 16 fractional bits:
  - `FIX32_FRACTIONAL_BITS` defaults to `16`
  - `FIX32_ONE` is `1 << FIX32_FRACTIONAL_BITS`
- The fractional-bit count is configurable from `1` through `30` with
  `FIX32_FRACTIONAL_BITS`; values outside that range are rejected at
  preprocessing time.
- `FIX32_INT_MIN` and `FIX32_INT_MAX` describe the whole-number range that can
  be scaled into the raw storage.
- `FIX32_HALF`, `FIX32_FRACTIONAL_MASK`, and `FIX32_INTEGER_MASK` expose the
  scale-derived constants used by the conversion and rounding helpers.
- `fix32_from_raw()` and `fix32_to_raw()` are identity conversions for code that
  needs direct access to the stored representation.
- `FIX32_VERSION_MAJOR`, `FIX32_VERSION_MINOR`,
  `FIX32_VERSION_REVISION`, and `FIX32_VERSION_STRING` identify the current
  header version, `0.3.0`.

The original target was a classic 16:16 layout because it is simple, familiar, and a good baseline for graphics-oriented arithmetic.

## Why single-header

The library is distributed as one header and follows the usual single-header
pattern: regular includes provide declarations, and one translation unit defines
`FIX32_IMPLEMENTATION` before including `fix32.h` to emit the function bodies.
For performance-sensitive code, defining `FIX32_STATIC_INLINE` keeps the old
`static inline` behavior in the including translation unit.

## Conversion and rounding policy

### `int -> fixed`

`fix32_from_int()` scales an integer by `FIX32_ONE`.

Rationale:

- Integer-to-fixed conversion is exact as long as the scaled value fits.
- The implementation uses a 64-bit intermediate for the scaling step to avoid overflowing during the multiply.

### `float` and `double -> fixed`

The direct constructors:

- `fix32_from_float()`
- `fix32_from_double()`

multiply by `FIX32_ONE` and convert to `fix32_t`. For values whose scaled
result is representable, the conversion truncates toward zero.

The explicit rounding constructors:

- `fix32_round_from_float()`
- `fix32_round_from_double()`

add or subtract half a raw unit before the cast:

```c
scaled + ((scaled >= 0) ? 0.5 : -0.5)
```

Rationale:

- No external math helpers are required.
- The behavior is explicit and easy to inspect.
- Callers can choose truncation or symmetric half-away-from-zero rounding
  directly.

### Rational -> fixed

`fix32_from_rational()` converts an integer numerator and denominator directly
to fixed-point:

```c
(numerator * FIX32_ONE) / denominator
```

The conversion truncates toward zero because it uses integer division. It is
useful when a scale, ratio, or reciprocal is already available as integer data
and the caller wants to avoid a temporary floating-point value.

### Conversion helper macros

`FIX32_FROM_INT` and `FIX32_FROM_RATIONAL` are direct aliases for the integer
and rational constructors. `FIX32_FROM_FLOAT`, `FIX32_FROM_DOUBLE`, and
`FIX32_TO_INT` use the rounded conversion policy by default. Defining
`FIX32_NO_ROUNDING` switches those three macros to truncating conversion.

`FIX32_TO_FLOAT`, `FIX32_TO_DOUBLE`, `FIX32_ITRUNC`, `FIX32_IFLOOR`, and
`FIX32_IROUND` expose the exact value and explicit integer conversion policies
independently of `FIX32_NO_ROUNDING`.

### `fixed -> int`

The module exposes:

- `fix32_trunc_to_int()`
- `fix32_floor_to_int()`
- `fix32_ceil_to_int()`
- `fix32_round_to_int()`

Rationale:

- These are common terminal conversions for pixel, coordinate, and sampling code.
- `fix32_trunc_to_int()` truncates toward zero.
- `fix32_floor_to_int()` and `fix32_ceil_to_int()` round toward negative and
  positive infinity respectively.
- `fix32_round_to_int()` uses the same half-away-from-zero policy as the
  explicit rounding constructors.

### `fixed -> float` and `double`

The module exposes:

- `fix32_to_float()`
- `fix32_floor_to_float()`
- `fix32_ceil_to_float()`
- `fix32_round_to_float()`
- `fix32_to_double()`
- `fix32_floor_to_double()`
- `fix32_ceil_to_double()`
- `fix32_round_to_double()`

Rationale:

- The direct conversions divide the raw value by `FIX32_ONE`.
- `double` can represent every 32-bit raw fixed-point value exactly; `float`
  may lose low-order precision.
- The floor, ceil, and round forms first produce the corresponding integer
  result and then cast it to `float` or `double`.

### Fixed-valued rounding

`fix32_floor()`, `fix32_ceil()`, and `fix32_round()` apply the corresponding
rounding policy while keeping the result in `fix32_t` representation.

## Unchecked input contract

The header performs no runtime validation and does not provide debug-only
assertions. Callers are responsible for keeping conversion and arithmetic
results within the representable range and for passing nonzero divisors.

The wider intermediates used by selected operations prevent specific
intermediate overflows, but they do not provide saturation or final-result
range checks. Floating-to-integer conversions also require the scaled input to
be representable by `fix32_t`.

## No external math-library functions

The library and benchmark intentionally avoid helpers such as:

- `lroundf()`
- `ceilf()`
- `floorf()`

Rationale:

- Keep the code self-contained and portable at the source level.
- Make the exact semantics visible in plain C.
- Avoid benchmarking the math library instead of the chosen numeric representation.

## Floor and ceil fast paths

`fix32_floor_to_int()` and `fix32_ceil_to_int()` support two modes.

### Fast path

Default:

```c
#define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 1
```

The floor implementation is:

```c
value >> FIX32_FRACTIONAL_BITS
```

The ceil implementation derives the same whole part with an arithmetic shift
and adds one when `FIX32_FRACTIONAL_MASK` shows a fractional part.

Rationale:

- On typical two's-complement targets, signed right shift is arithmetic and gives the desired floor behavior directly.
- The same shifted whole part is a convenient basis for ceiling.
- This is the default fast path.

### Portable fallback

Enabled with:

```c
#define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 0
```

Both functions use division and remainder, then correct the truncated whole
part according to the sign and whether a fraction is present.

Rationale:

- Right-shifting a negative signed value is implementation-defined in C99.
- The fallback keeps a standards-safe option available.

## Bitmask usage

The header exposes:

```c
#define FIX32_FRACTIONAL_MASK (FIX32_ONE - 1)
#define FIX32_INTEGER_MASK    (~FIX32_FRACTIONAL_MASK)
```

Rationale:

- The fixed-point scale is a power of two, so the low raw bits naturally hold the fractional field.
- A mask gives a fast way to test whether a value has a fractional part.

Current use:

- The arithmetic-shift `ceil_to_int()` fast path uses `FIX32_FRACTIONAL_MASK` to detect whether any fractional bits are present.
- `fix32_floor()` clears the fractional field with `FIX32_INTEGER_MASK`.
- `fix32_ceil()` clears the fractional field and adds `FIX32_ONE` when a
  fraction is present.

Why it is not used everywhere:

- Masks are excellent for fractional-bit extraction and “has fraction” tests.
- They are less universally helpful for higher-level arithmetic rules, especially when negative-value semantics need to stay explicit.
- Modern compilers already optimize many power-of-two operations well, so masks were used only where they also improve clarity.

## Basic arithmetic

`fix32_add()` and `fix32_sub()` operate directly on the raw fixed-point values.
`fix32_mul_by_int()` scales a fixed-point value by an integer without changing
the radix position.

These operations do not perform overflow checks. When `FIX32_USE_64_BIT` is
nonzero, `fix32_mul_by_int()` uses a 64-bit product before converting the result
back to `fix32_t`; otherwise it uses a 32-bit multiplication.

## Multiplication design

### Why fixed-point multiply is special

If a fixed-point format has:

- `I` magnitude bits to the left of the radix point
- `F` fractional bits

then each raw operand uses about `I + F` magnitude bits. Multiplying two raw operands temporarily produces a value with roughly:

```text
2 * (I + F)
```

magnitude bits before the final scaling by `2^F`.

This is why multiplication is the main place where an intermediate wider than 32 bits may be needed.

### Default multiply

`fix32_mul()` defaults to a 64-bit intermediate and defined signed division:

```c
((int64_t)left * (int64_t)right) / FIX32_ONE
```

Rationale:

- Every product of two `int32_t` operands fits in the 64-bit intermediate.
- Truncates negative results toward zero without relying on signed-shift
  behavior.
- The caller must still ensure that the scaled result fits in `fix32_t`.

### Optional signed-shift multiply scaling

Arithmetic right-shift scaling can be enabled with:

```c
#define FIX32_USE_SIGNED_SHIFT_MUL 1
```

This path can be benchmarked with:

```sh
make run-benchmark-shift-mul
```

For negative products, signed right shift is implementation-defined in C99 and
typically rounds toward negative infinity on two's-complement targets. That can
differ by one raw unit from the default division path, which truncates toward
zero.

### Optional 32-bit multiply path

Can be forced with:

```c
#define FIX32_USE_64_BIT 0
```

With `FIX32_USE_64_BIT=0`, `fix32_mul()` narrows its computed product to
`int32_t` before scaling, `fix32_mul_by_int()` uses a 32-bit multiplication,
and `fix32_round_to_int()` uses a 32-bit temporary.

The caller must ensure those narrowed intermediates fit. The library does not
prove or check that constraint. In particular, the current `fix32_mul()`
expression still performs a 64-bit multiplication before storing the result in
the 32-bit temporary; this mode changes the stored intermediate and related
helper paths rather than eliminating every 64-bit operation from the header.

### Optional multiply-range hint

The library supports:

```c
#define FIX32_INTEGER_BITS ...
```

This macro is only a hint for multiply-path selection. It does **not** change the stored format.

If `FIX32_USE_64_BIT` is not defined explicitly, and `FIX32_INTEGER_BITS` is defined, the header derives the multiply path from:

```c
((FIX32_INTEGER_BITS + FIX32_FRACTIONAL_BITS) > 15)
```

Rationale:

- The exact condition for a 32-bit raw product to overflow is:

```c
2 * (FIX32_INTEGER_BITS + FIX32_FRACTIONAL_BITS) > 31
```

- Because the left side is always even, that is exactly equivalent to:

```c
(FIX32_INTEGER_BITS + FIX32_FRACTIONAL_BITS) > 15
```

- The shorter test says the same thing with less clutter.

Important detail:

- If no multiply-range hint is provided, the library retains the full 64-bit
  product through the scaling step by default.
- The earlier idea of defaulting the hint to `31 - FIX32_FRACTIONAL_BITS` was rejected because it made the auto-selection meaningless: it would always force 64-bit multiply.
- The hint is used only for compile-time path selection. It does not change the
  fixed-point format and is not enforced at runtime.

## Where 64-bit intermediates are used

64-bit arithmetic is used selectively.

It is always used by:

- `fix32_from_int()` while scaling the integer input
- the multiplication expression in `fix32_mul()`
- the default `fix32_div()` numerator scaling and quotient

When `FIX32_USE_64_BIT` is nonzero, it is also used for:

- retaining the full product in `fix32_mul()` until after scaling
- `fix32_mul_by_int()`
- the temporary in `fix32_round_to_int()`

Operations that remain narrow and simple:

- `fix32_add()`
- `fix32_sub()`
- `fix32_div_by_int()`
- `fix32_floor_to_int()` fast path
- `fix32_ceil_to_int()`
- `fix32_to_float()`

## Division and reciprocal strategy

The header now provides:

- `fix32_div_by_int()`
- `fix32_div()`
- `fix32_reciprocal_by_int()`
- `fix32_reciprocal()`

The integer-divisor helpers operate directly on the raw fixed-point value:
`fix32_div_by_int()` divides a fixed value by an integer, while
`fix32_reciprocal_by_int()` computes `FIX32_ONE / value`.

### Direct division

By default, `fix32_div()` uses:

```c
((int64_t)numerator * (int64_t)FIX32_ONE) / denominator
```

Rationale:

- This is the clearest primitive for fixed/fixed division.
- It keeps division semantics explicit in the API.
- It avoids forcing the caller to hide every divide inside an ad hoc reciprocal-multiply pattern.
- Multiplication by the compile-time power-of-two scale is normally optimized to the same machine operation as an explicit left shift.

The original signed-shift scaling path can be enabled before including the header:

```c
#define FIX32_USE_SIGNED_SHIFT_DIV 1
```

This path can be benchmarked with:

```sh
make run-benchmark-shift-div
```

The scalar division row uses nonnegative numerators so this benchmark does not
invoke the signed-left-shift path's undefined behavior.

That path computes:

```c
((int64_t)numerator << FIX32_FRACTIONAL_BITS) / denominator
```

Left-shifting a negative signed value is undefined behavior in C99. This option
is valid only when every numerator passed to `fix32_div()` is nonnegative. The
default value is `0`.

Rounding detail:

- `fix32_div()` inherits the truncation behavior of integer division in C99.
- This means direct fixed division and a float reciprocal converted back through
  `fix32_round_from_float()` can differ because they use different rounding
  paths and the float path has limited precision.

### Precomputed reciprocal

`fix32_reciprocal()` is implemented in terms of `fix32_div()`, and is intended for cases where a divisor is reused:

```c
reciprocal = fix32_reciprocal(divisor);
result = fix32_mul(value, reciprocal);
```

Rationale:

- If the same divisor is used repeatedly, precomputing the reciprocal once can be more useful than issuing a full fixed/fixed division every time.
- This matches the intended optimization strategy for repeated scaling or normalization workloads.

Why the library keeps both:

- Direct division is the clearer primitive for a one-off quotient.
- Reciprocal multiply is a secondary optimization path for repeated divisions by the same value.
- Computing the reciprocal on the fly was explicitly avoided because it would only hide the division cost rather than reduce it.

Benchmark note:

- The scalar benchmark now measures reciprocal computation itself as a separate case.
- The fixed path uses `fix32_reciprocal(x)`.
- The floating-point baseline computes `1.0f / x` in float and then converts that reciprocal back to fixed with `fix32_round_from_float()`.
- This keeps both paths comparable because they produce the same final fixed-point representation.
- For the benchmark's bounded inputs, non-exact reciprocals may differ by one
  raw LSB because the fixed path truncates through integer division while
  `fix32_round_from_float()` rounds to nearest.

## Why the benchmark is split into scalar and workload sections

The benchmark has four layers.

### 1. Scalar microbenchmarks

These measure the isolated cost of:

- integer, float, and double conversion to fixed-point
- add and subtract
- fixed and integer multiply/divide operations
- integer and fixed-point reciprocals
- multiply by a precomputed reciprocal
- trunc/ceil/floor/round to integer
- fixed-valued floor/ceil/round
- value/ceil/floor/round conversion to float and double

Rationale:

- Gives a clean view of the raw operator cost.
- Makes it easy to spot where fixed point helps or hurts.

### 2. Virtual DDA line drawing

The line benchmark computes the stepping logic of a DDA rasterizer without touching a framebuffer.

Rationale:

- DDA is a classic graphics workload.
- It emphasizes one-time setup plus repeated addition and rounding.
- It shows a realistic case where fixed-point arithmetic can be attractive without the benchmark being dominated by memory stores.

Implementation notes:

- The step is computed once.
- The loop advances by repeated addition.
- Pixel coordinates are converted with rounding.
- A checksum sink prevents the compiler from deleting the work.

### 3. Virtual sprite scaling

The sprite benchmark computes source sampling coordinates for increasingly large destination sizes, again without actual drawing.

Rationale:

- Sprite scaling is another representative graphics workload.
- It stresses stepping, floor conversion, and repeated coordinate accumulation.
- Avoiding actual memory reads/writes keeps the benchmark focused on numeric representation cost.

Implementation notes:

- Source step values are computed once.
- The inner loops advance source coordinates by addition.
- Sampling uses floor conversion to derive source texel indices.
- Complexity grows with destination pixel count.

### 4. Virtual rotoscaling

The rotoscaler benchmark computes rotated and scaled source sampling coordinates for increasingly large destination sizes, again without actual drawing.

Rationale:

- It extends the sprite-scaling test into affine blitting territory.
- It stresses the same stepping and floor conversion behavior as the scaler, but with rotated step vectors and a more realistic affine setup phase.
- It is a closer match for software sprite rotation, affine effects, and camera-space blit paths than the axis-aligned scaler alone.

Implementation notes:

- Rotation is represented by predeclared cosine/sine pairs rather than runtime trigonometric functions.
- Each case computes rotated/scaled X and Y step vectors once, then advances row origins and per-pixel coordinates by repeated addition.
- Sampling still uses floor conversion to derive source texel indices.
- Complexity grows with destination pixel count.

## Why complexity is increased gradually

The benchmark grows workload size rather than using a single case.

Rationale:

- Small workloads show setup overhead more clearly.
- Large workloads show steady-state per-iteration behavior.
- DDA uses increasing line lengths.
- Sprite scaling uses progressively larger source and destination sizes.
- Rotoscaling uses progressively larger source and destination sizes with representative rotation angles.

This makes it easier to see whether a result is sensitive to fixed overhead or remains stable as the amount of arithmetic grows.

## Deliberate omissions

The module currently does **not** try to provide:

- saturation arithmetic
- overflow detection
- transcendental functions
- full rendering code

Rationale:

- The module was intentionally kept small and focused on the operations under study.
- The benchmarks are designed to compare numeric cost, not renderer throughput or memory bandwidth.

## Future improvements and ideas

These are possible future directions for the fixed-point module and for engine subsystems that could make practical use of it. They are design notes, not current commitments.

### 1. A targeted fixed-point math layer

A fixed-point vector/math layer could make sense, but it should be specialized rather than trying to replace a full floating-point math stack one-to-one.

Good candidates:

- `fx_vec2` for 2D positions, velocities, and source-coordinate stepping
- `fx_rect` and `fx_aabb2` for clipping, collision, and bounds work
- `fx_affine2` or `fx_transform2` for sprite scaling, rotoscaling, and affine blits
- `fx_uvstep` or `fx_span` style helpers for raster/span interpolation

Rationale:

- The current benchmark results strongly favor fixed point in repeated-addition, floor-heavy, integer-facing inner loops.
- These types would map naturally onto the kinds of coordinate-generation workloads already benchmarked.
- A specialized layer is less risky than building a complete fixed-point equivalent of a general-purpose float math library.

### 2. Deterministic gameplay simulation

Fixed point may be a strong fit for deterministic gameplay systems, especially when exact replay or synchronization matters.

Potential candidates:

- lockstep multiplayer simulation
- replay systems
- deterministic input recording/playback
- rollback-friendly 2D gameplay state updates

Rationale:

- Fixed-point arithmetic can make cross-platform behavior easier to constrain than a float-heavy simulation.
- The integer-facing nature of the representation can help when exact reproducibility matters more than raw mathematical flexibility.

### 3. 2D movement, collision, and platformer physics

A future fixed-point extension could target simple 2D simulation systems such as:

- character controllers
- tile collision
- swept AABB movement
- subpixel movement
- slopes, ladders, conveyors, and moving platforms

Rationale:

- These systems are often heavy on accumulation, clipping, and integer-facing position resolution.
- They are usually a better match for fixed point than a full rigid-body physics stack.

### 4. Camera, scrolling, and parallax systems

Other likely beneficiaries:

- camera tracking
- parallax scrolling
- world-to-screen conversion
- layered background motion
- screen-space shake or scripted offsets

Rationale:

- These systems typically rely on repeated addition, interpolation, and final conversion to pixel coordinates.
- That arithmetic profile is close to the workloads where fixed point already benchmarked well.

### 5. Rasterization and software rendering helpers

The benchmark results suggest further exploration in:

- span walkers
- edge walkers
- affine texture-coordinate interpolation
- software texture mapping
- sprite UV stepping
- software rasterization support code

Rationale:

- DDA, scaling, and rotoscaling all point toward fixed point being a good fit for source-coordinate generation and raster stepping.
- These areas often share the same "setup once, add many times, convert to integer sample indices" structure.

### 6. Particles and simple 2D VFX

Possible future uses:

- CPU particle simulation
- simple ballistic motion
- emitter stepping
- deterministic 2D VFX timing and movement

Rationale:

- These systems can often be expressed mostly in terms of accumulation and quantized output.
- They may benefit if determinism or integer-facing rendering output is more important than general-purpose continuous math.

### 7. UI animation and tween systems

Possible use cases:

- HUD and UI interpolation
- tweening of screen-space values
- deterministic timeline-driven animation
- menu and overlay transitions that ultimately land on pixel coordinates

Rationale:

- These systems often end in integer or snapped screen coordinates.
- Fixed-point can be a reasonable option if the pipeline wants deterministic stepping and simple quantization behavior.

### 8. Audio as a possible but less certain area

A fixed-point path might also be explored for:

- audio mixing
- resampling
- sample-position stepping

Rationale:

- Historically, fixed point has been common in audio code.
- On modern systems, float is often already very competitive and simpler to work with.
- This area is therefore a possible future experiment, but not an obvious next target.

### 9. Systems where float likely remains preferable

The current design notes suggest that fixed point should probably **not** be the default choice for:

- general 3D transforms
- quaternions
- matrix-heavy camera code
- skeletal animation
- rigid-body physics
- trig-heavy systems
- code that already lives naturally in float and is not hot enough to justify format conversion

Rationale:

- The current scalar benchmark does not show fixed point winning on general multiply/divide-heavy math.
- These areas tend to demand broader dynamic range, more complex composed math, and less integer-facing output than the blitting-style workloads.

### 10. Likely architectural direction

The most practical future direction is a hybrid engine strategy:

- use float for general-purpose engine math and higher-level setup work
- use fixed point for deterministic subsystems and hot inner loops that repeatedly step toward integer-facing results

Examples of a mixed approach:

- compute transforms and rotation setup in float
- convert final step vectors and start coordinates once into fixed
- run the hot blit, scaler, or rotoscaler inner loop in fixed

Rationale:

- This matches the current benchmark results much better than an "all fixed everywhere" design.
- It keeps float where it is convenient, while exploiting fixed point where the arithmetic pattern is especially favorable.

### 11. Benchmark-guided extension strategy

Any future subsystem added on top of this library should ideally be benchmarked in the same style before it is treated as a clear win.

Good future benchmark candidates:

- fixed `vec2` stepping and interpolation
- tile collision / controller update loops
- affine span walkers
- software texture-mapping spans
- particle update loops

Rationale:

- The current benchmark already showed that the answer depends strongly on the arithmetic pattern.
- A benchmark-first approach helps avoid overgeneralizing from one favorable workload to unrelated systems.

## Summary

The library favors:

- a small single-header C99 implementation
- explicit rounding logic in plain C
- fast paths where the performance win is meaningful
- compile-time switches where portability and speed trade off directly
- wide default intermediates for fixed multiplication and division
- caller-owned range, overflow, and divisor validity

The benchmark favors:

- isolating arithmetic cost from rendering cost
- comparing both microbenchmarks and graphics-style workloads
- increasing workload complexity to reveal steady-state behavior

Overall, the design treats fixed point as a practical engineering tradeoff: use the fast path when the platform and operand range are known, select the portable floor/ceil fallback when required, and keep all inputs inside the unchecked arithmetic domain.
