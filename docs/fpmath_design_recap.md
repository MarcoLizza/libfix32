# fpmath design recap

This document summarizes the design choices and rationale behind the header-only fixed-point module in `fpmath.h` and the associated benchmark suite in `src/bench`.

## Goals

- Keep the library in straight C99.
- Keep the library header-only, with `static inline` helpers and no external dependencies.
- Compare a signed 32-bit fixed-point representation against floating point for conversion-heavy and graphics-style workloads.
- Avoid math-library helpers such as `lroundf()` and `ceilf()` so the implementation stays self-contained.

## Representation

- The storage type is `int32_t` (`fix32_t`).
- The default format is 16 fractional bits:
  - `FIX32_FRACTION_BITS` defaults to `16`
  - `FIX32_SCALE` is `1 << FIX32_FRACTION_BITS`
- The fractional-bit count is configurable through `FIX32_FRACTION_BITS`. The implementation still assumes a signed 32-bit raw storage type.

The original target was a classic 16:16 layout because it is simple, familiar, and a good baseline for graphics-oriented arithmetic.

## Why header-only

- Small helper functions benefit from inlining.
- It avoids a separate compilation unit.
- It keeps the benchmark and the library easy to drop into small C projects.

## Conversion and rounding policy

### `int -> fixed`

`fix32_from_int()` scales an integer by `FIX32_SCALE`.

Rationale:

- Integer-to-fixed conversion is exact as long as the scaled value fits.
- The implementation uses a 64-bit intermediate for the scaling step to avoid overflowing during the multiply.

### `float -> fixed`

`fix32_from_float()` uses:

```c
scaled + ((scaled >= 0.0f) ? 0.5f : -0.5f)
```

before the cast.

Rationale:

- No external math helpers are required.
- The behavior is explicit and easy to inspect.
- The chosen policy is symmetric half-away-from-zero rounding.

### `fixed -> int`

The module exposes:

- `fix32_floor_to_int()`
- `fix32_ceil_to_int()`
- `fix32_round_to_int()`

Rationale:

- These are common terminal conversions for pixel, coordinate, and sampling code.
- `round_to_int()` uses the same half-away-from-zero behavior as `from_float()`.

### `fixed -> float`

The module exposes:

- `fix32_to_float()`
- `fix32_floor_to_float()`
- `fix32_ceil_to_float()`
- `fix32_round_to_float()`

Rationale:

- These let the benchmark compare exact value conversion against quantized float outputs.
- The `*_to_float()` rounded forms are implemented by first producing the integer result, then casting to float, so the rounding policy stays consistent.

## No external math-library functions

The library and benchmark intentionally avoid helpers such as:

- `lroundf()`
- `ceilf()`
- `floorf()`

Rationale:

- Keep the code self-contained and portable at the source level.
- Make the exact semantics visible in plain C.
- Avoid benchmarking the math library instead of the chosen numeric representation.

## Debug-time validation

The header now supports:

```c
#define FIX32_ENABLE_DEBUG_CHECKS ...
```

Default behavior:

- enabled when `NDEBUG` is not defined
- disabled when `NDEBUG` is defined

Rationale:

- Catch out-of-range inputs during development without forcing release builds to pay for the checks.
- Make configuration mistakes visible early, especially when experimenting with different fractional widths or 32-bit-only multiply mode.

What is checked in debug builds:

- `fix32_from_int()` asserts that the integer input fits after scaling.
- `fix32_from_float()` asserts that the rounded scaled value can still fit in the 32-bit raw storage.
- `fix32_add()` asserts that the raw sum fits in the 32-bit storage.
- `fix32_mul()` asserts either that the scaled 64-bit result fits, or, in 32-bit multiply mode, that the raw product fits before the shift.
- `fix32_div_by_int()` and `fix32_div()` assert that the divisor is nonzero.
- `fix32_div()` asserts that the scaled quotient fits the raw storage.

Benchmark note:

- The benchmark explicitly uses `fix32_round_from_float()` when it needs nearest-value conversion.

Interaction with the multiply-range hint:

- If `FIX32_MUL_INTEGER_BITS` is defined, debug builds also assert that operands passed to `fix32_mul()` stay within the promised range.
- This hint remains a programmer promise about expected multiply inputs; it is not the actual format definition.

## Floor fast path vs portable fallback

`fix32_floor_to_int()` supports two modes.

### Fast path

Default:

```c
#define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 1
```

Implementation:

```c
value >> FIX32_FRACTION_BITS
```

Rationale:

- On typical two's-complement targets, signed right shift is arithmetic and gives the desired floor behavior directly.
- This is faster and simpler for the common case.

### Portable fallback

Enabled with:

```c
#define FIX32_USE_ARITHMETIC_SHIFT_FLOOR 0
```

Implementation uses division and remainder to correct negative non-integer values.

Rationale:

- Right-shifting a negative signed value is implementation-defined in C99.
- The fallback keeps a standards-safe option available.

## Bitmask usage

The header now exposes:

```c
#define FIX32_FRAC_MASK (FIX32_SCALE - 1)
```

Rationale:

- The fixed-point scale is a power of two, so the low raw bits naturally hold the fractional field.
- A mask gives a fast way to test whether a value has a fractional part.

Current use:

- The arithmetic-shift `ceil_to_int()` fast path uses `FIX32_FRAC_MASK` to detect whether any fractional bits are present.

Why it is not used everywhere:

- Masks are excellent for fractional-bit extraction and “has fraction” tests.
- They are less universally helpful for higher-level arithmetic rules, especially when negative-value semantics need to stay explicit.
- Modern compilers already optimize many power-of-two operations well, so masks were used only where they also improve clarity.

## Multiplication design

### Why fixed-point multiply is special

If a fixed-point format has:

- `I` magnitude bits to the left of the radix point
- `F` fractional bits

then each raw operand uses about `I + F` magnitude bits. Multiplying two raw operands temporarily produces a value with roughly:

```text
2 * (I + F)
```

magnitude bits before the final right shift by `F`.

This is why multiplication is the main place where an intermediate wider than 32 bits may be needed.

### Default multiply

`fix32_mul()` defaults to a 64-bit intermediate:

```c
((int64_t)left * (int64_t)right) >> FIX32_FRACTION_BITS
```

Rationale:

- Safe default for general use.
- Prevents overflow in the raw product before the post-multiply shift.

### Optional 32-bit multiply path

Can be forced with:

```c
#define FIX32_USE_64BIT_MUL 0
```

Rationale:

- Some constrained use cases can guarantee smaller operand ranges.
- In those cases the programmer may prefer to avoid 64-bit arithmetic entirely.

Tradeoff:

- The library does not prove safety for this mode.
- The caller is responsible for ensuring that the raw 32-bit product fits before the shift.

### Optional multiply-range hint

The library supports:

```c
#define FIX32_MUL_INTEGER_BITS ...
```

This macro is only a hint for multiply-path selection. It does **not** change the stored format.

If `FIX32_USE_64BIT_MUL` is not defined explicitly, and `FIX32_MUL_INTEGER_BITS` is defined, the header derives the multiply path from:

```c
((FIX32_MUL_INTEGER_BITS + FIX32_FRACTION_BITS) > 15)
```

Rationale:

- The exact condition for a 32-bit raw product to overflow is:

```c
2 * (FIX32_MUL_INTEGER_BITS + FIX32_FRACTION_BITS) > 31
```

- Because the left side is always even, that is exactly equivalent to:

```c
(FIX32_MUL_INTEGER_BITS + FIX32_FRACTION_BITS) > 15
```

- The shorter test says the same thing with less clutter.

Important detail:

- If no multiply-range hint is provided, the library falls back to safe 64-bit multiply by default.
- The earlier idea of defaulting the hint to `31 - FIX32_FRACTION_BITS` was rejected because it made the auto-selection meaningless: it would always force 64-bit multiply.

Debug-time consequence:

- If the programmer supplies `FIX32_MUL_INTEGER_BITS`, debug builds treat that as a promise and assert it at runtime when `fix32_mul()` is used.

## Where 64-bit intermediates are used

64-bit arithmetic is used selectively, not everywhere.

Current rationale:

- `fix32_from_int()` uses a 64-bit intermediate for scaling.
- `fix32_round_to_int()` uses a 64-bit temporary while adding/subtracting half a unit.
- `fix32_mul()` uses 64-bit by default, but can be switched to 32-bit when the caller guarantees a safe range.

Operations that remain narrow and simple:

- `fix32_add()`
- `fix32_div_by_int()`
- `fix32_floor_to_int()` fast path
- `fix32_ceil_to_int()`
- `fix32_to_float()`

## Division and reciprocal strategy

The header now provides:

- `fix32_div()`
- `fix32_reciprocal()`

### Direct division

By default, `fix32_div()` uses:

```c
((int64_t)numerator * (int64_t)FIX32_SCALE) / denominator
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

That path computes:

```c
((int64_t)numerator << FIX32_FRACTION_BITS) / denominator
```

Left-shifting a negative signed value is undefined behavior in C99. Enable this option only when relying on a specific compiler behavior is acceptable. The default value is `0`.

Rounding detail:

- `fix32_div()` inherits the truncation behavior of integer division in C99.
- This means direct fixed division and a float reciprocal converted back through `fix32_from_float()` can differ by one raw least-significant unit on values that are not exactly representable.

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

- Direct division is the safer and more obvious primitive.
- Reciprocal multiply is a secondary optimization path for repeated divisions by the same value.
- Computing the reciprocal on the fly was explicitly avoided because it would only hide the division cost rather than reduce it.

Benchmark note:

- The scalar benchmark now measures reciprocal computation itself as a separate case.
- The fixed path uses `fix32_reciprocal(x)`.
- The floating-point baseline computes `1.0f / x` in float and then converts that reciprocal back to fixed with `fix32_from_float()`.
- This keeps both paths comparable because they produce the same final fixed-point representation.
- For non-exact reciprocals, the two paths may differ by one raw LSB because the fixed path truncates through integer division while `fix32_from_float()` rounds to nearest.

## Why the benchmark is split into scalar and workload sections

The benchmark has four layers.

### 1. Scalar microbenchmarks

These measure the isolated cost of:

- int/float to representation
- add
- multiply
- reciprocal to fixed
- divide
- multiply by precomputed reciprocal
- ceil/floor/round to int
- value/ceil/floor/round to float

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

- a small header-only C99 implementation
- explicit rounding logic in plain C
- fast paths where the performance win is meaningful
- compile-time switches where portability and speed trade off directly
- safe defaults unless the programmer explicitly promises tighter operand bounds

The benchmark favors:

- isolating arithmetic cost from rendering cost
- comparing both microbenchmarks and graphics-style workloads
- increasing workload complexity to reveal steady-state behavior

Overall, the design treats fixed point as a practical engineering tradeoff: use the fast path when the platform and operand range are known, but keep safe and portable fallbacks available.
