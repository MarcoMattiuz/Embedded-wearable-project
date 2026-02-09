# `hr_logic.c` – Heart‑rate logic (MAX30102)

This module processes the raw **IR/RED** samples from the MAX30102 and:

1. estimates and removes the **DC** component (baseline)
2. filters the **AC** component using a **low‑pass FIR**
3. detects heart beats (zero‑crossing + amplitude + refractory period)
4. computes instantaneous **BPM** and a **moving average** BPM

> Note: most of the logic is implemented in **fixed‑point Q15** for MCU efficiency. (simulates decimals using integers with the shift bit-bit operations << & >>)

---

## DC component removal

### `averageDCEstimator(int32_t *p, uint16_t x)`

Estimates the signal baseline (DC) using an **IIR / exponential moving average** in fixed‑point:

- `p` holds the internal state in Q15 format (scaled by `2^15`).
- at every new sample `x`, it updates `*p` with:
  - error = `(x << 15) - *p`
  - correction = `error >> 4` → factor `1/16` (time constant)
- returns the estimated DC in normal units (`*p >> 15`).

Practical effect:
- slowly follows baseline variations (ambient light, finger pressure, etc.)
- preserves the pulsatile (AC) variations after subtraction.

### `get_IR_AC(uint32_t sample)` / `get_RED_AC(uint32_t sample)`

Pipeline to obtain the filtered AC signal:

1. `dc_estimate = averageDCEstimator(&*_dc_estimate, sample)`
2. `ac = sample - dc_estimate` → baseline removal
3. `return lowPassFIRFilter*(ac)` → smoothing / low‑pass filtering of AC

Result: a signal centered around 0, cleaner and suitable for beat detection.

---

## Low‑pass FIR

### `lowPassFIRFilterIR(int16_t din)` / `lowPassFIRFilterRED(int16_t din)`

These implement a **low‑pass FIR filter** using:

- a circular buffer `cbuf[32]` with index `offset`
- filter coefficients `FIRCoeffs[12]`
- 32‑bit accumulation (`int32_t z`) with `mul16()` multiplications
- Q15 output → returned as `z >> 15`

The implementation exploits the **symmetry** of the coefficients:

- one central term (index 11)
- for `i = 0..10`, pairs of symmetric samples are summed from the buffer

Purpose:
- attenuates high‑frequency noise
- stabilizes amplitude and zero‑crossings of the AC signal.

- found in the Sparkfun library documentation

---

## Beat detection

### `beat_detected(int16_t ir_ac)`

Detects a beat by observing the filtered IR AC signal.
The main rule is a **positive zero‑crossing**:

- if `previous < 0` and `current >= 0` → start of a new cycle

At that moment it evaluates:

1. **Cycle amplitude**: `amplitude = IR_AC_Max - IR_AC_Min`
   - `IR_AC_Max` and `IR_AC_Min` are continuously updated during the cycle
2. **Refractory period**: `delta = sample_counter - lastBeatSample`
   - if `delta < MIN_BEAT_INTERVAL` → discard (prevents double triggers)
3. **Amplitude threshold**:
   - accepted only if `amplitude` is within a reasonable range (e.g. `> 100` and `< 2000`)

If the beat is accepted:
- returns `true`
- resets `IR_AC_Max/Min` for the next cycle

---

## BPM calculation and moving average

### `calculateBPM(int16_t ir_ac, int16_t *BPM, int16_t *AVG_BPM)`

1. increments `sample_counter` (global sample counter)
2. calls `beat_detected(ir_ac)`
3. on a detected beat:
   - stores `now = sample_counter`
   - if this is the **first beat**, initializes `lastBeatSample` and exits
   - otherwise computes `delta = now - lastBeatSample` (distance in samples)

#### Instantaneous BPM

With a constant sample rate `SAMPLE_RATE` (50 Hz):

`currBPM = 60 * SAMPLE_RATE / delta`

Accepted only if within a plausible range (e.g. `40..210`).

#### Moving average (AVG_BPM)

- stores `currBPM` in a circular buffer `rates[]`
- uses `rate_size` as the window length (currently 4)
- computes `AVG_BPM` as the average of the last `rate_size` values

Output:
- `*BPM` = instantaneous BPM
- `*AVG_BPM` = averaged BPM (more stable)

---

## Key variables

- `*_dc_estimate` (int32) : DC estimator state in Q15
- `FIRCoeffs[12]`         : low‑pass FIR coefficients
- `sample_counter`        : global sample counter for timing
- `MIN_BEAT_INTERVAL`     : minimum distance (in samples) between beats
- `IR_AC_Max/Min`         : cycle extremes used to estimate amplitude
- `rates[]`, `rate_size`  : buffer and window for BPM averaging