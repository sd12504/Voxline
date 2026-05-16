# VOXLINE — Implementation Plan

## Full Audit: Current State

### APVTS → DSP Connection

| Parameter | APVTS | DSP | UI | Status |
|-----------|-------|-----|----|--------|
| inputGain | ✅ | ✅ inputGainSmoothed | ✅ SliderAttachment | ✅ |
| autoGain | ✅ bool | ✅ auto output gain compensation | ✅ ToggleAttachment | ✅ |
| polish | ✅ | ✅ Scales tone/comp/drive intensity | ✅ SliderAttachment | ✅ |
| body | ✅ | ✅ IIR peak @ 200Hz Q0.6 | ✅ SliderAttachment | ✅ |
| clarity | ✅ | ✅ IIR peak @ 3.5kHz Q1.0 | ✅ SliderAttachment | ✅ |
| air | ✅ | ✅ IIR high shelf @ 7kHz Q0.7 | ✅ SliderAttachment | ✅ |
| smooth | ✅ | ✅ IIR high shelf cut @ 6kHz Q0.7 | ✅ SliderAttachment | ✅ |
| comp | ✅ | ✅ Single compressor | ✅ SliderAttachment | ✅ |
| drive | ✅ | ✅ Soft clip + pre-gain | ✅ SliderAttachment | ✅ |
| outputGain | ✅ | ✅ outputGainSmoothed | ✅ SliderAttachment | ✅ |
| bypass | ✅ bool | ✅ Crossfade blend | ✅ ToggleAttachment | ✅ |
| listen | ✅ bool | ⚠️ Difference monitor only | ✅ ToggleAttachment | ⚠️ |
| spaceAmount | ✅ | ✅ Delay-based spatial | ✅ SliderAttachment | ⚠️ |
| spaceType | ✅ int 0-3 | ✅ 4 switch cases | ✅ ComboBox listener | ⚠️ |

### Feature Completeness

| Feature | Status | Notes |
|---------|--------|-------|
| cleanMode | ❌ Not implemented | Layout bounds exist, no APVTS, no DSP, no UI component |
| saturation/warmth | ❌ Not implemented | No parameter, no DSP |
| A/B | ✅ Complete | Snapshot capture/apply, toggle working |
| Presets | ✅ Complete | 9 presets, full parameter apply |
| Theme toggle | ✅ Complete | Light/Dark working |
| Meters (output/GR) | ✅ Complete | TimerCallback updating |
| Input LED dots | ✅ Complete | 7-dot level indicator |

### DSP Quality Notes

**TONE (body/clarity/air/smooth):**
- Fixed frequencies, fixed Q, only gain varies
- Each filter's range is relatively narrow (e.g. body: -4~+5dB)
- Polish scales all of them uniformly

**COMP:**
- Single compressor model
- Attack/release hardcoded (10ms attack, 80ms release)
- No ratio/threshold separation — both derived from `comp` amount
- No model switching

**DRIVE:**
- Simple `tanh()` soft clip
- Pre-gain up to 12dB, normalized
- No saturation character, no multi-stage shaping

**SPACE (case 3 "Vocal Space"):**
- This is effectively a room reverb — violates design direction
- wet mix 0.22, fb 0.22, no HPF/LPF applied in default case
- Should be removed

---

## Staged Implementation Plan

### ✅ Stage 1: Structural Cleanup (Complete)
*No new features. Fix broken/incomplete things.*

1. **Remove SPACE case 3** (Vocal Space / room reverb)
   - Reduce spaceType range from 0-3 to 0-2
   - Keep: Tight Ambience (0), Filtered Slap (1), Stereo Wide (2)
   - Update APVTS parameter range (int 0→2)
   - Update UI combo box items

2. **Implement cleanMode**
   - APVTS: new bool param `cleanMode`
   - DSP: simple highpass + noise gate (or just the layout's existing LED dots area)
   - UI: add ToggleButton at cleanModeBounds

3. **Fix Listen mode**
   - Currently: `(sample - drySample) * 2.0f` (difference/solo)
   - Should: output processed signal when listen is on, silence when off? Or proper solo?
   - Need to clarify intended behavior

### ✅ Stage 2: DSP Strengthening (Complete)
*Improve existing DSP without adding UI controls.*

1. **TONE — Widen filter ranges**
   - body: ±8dB instead of -4~+5dB
   - clarity: ±8dB instead of -3~+6dB
   - air: ±8dB instead of -3~+7dB
   - smooth: wider cut range, -8~0dB instead of -6~0dB
   - No UI changes, no new params

2. **COMP — Improve response**
   - Add variable attack/release based on comp amount (faster attack at higher settings)
   - Softer knee at lower settings

3. **DRIVE — Add saturation character**
   - Keep `tanh()` as base
   - Add asymmetric clipping (different saturation for positive/negative)
   - Or multi-stage saturation curve
   - Still one knob, no new UI

### ✅ Stage 3: SPACE Refinement (Complete)
*Make SPACE actually good.*

1. **Tight Ambience (0):**
   - Short taps: 12ms / 22ms / 34ms / 48ms
   - Low feedback (0.10-0.15)
   - Heavy filtering: HPF 250Hz, LPF 7kHz
   - Conservative mix (0.12-0.18)

2. **Filtered Slap (1):**
   - ~105ms + ~140ms taps
   - Moderate feedback (0.20-0.25)
   - Filtering: HPF 200Hz, LPF 5kHz
   - Slight pitch modulation for analogue feel?

3. **Stereo Wide (2):**
   - Cross-feed between channels
   - Short ambience + Haas-style delay (~15-25ms)
   - HPF 220Hz, LPF 8kHz
   - Mono-compatible check

### ✅ Stage 4: Polish & Presets (Complete)

1. Review and tune all 9 presets with new DSP ranges
2. Ensure POLISH macro works well across all presets
3. A/B behavior: ensure snapshot captures all params including space
4. Update preset names/values if needed

---

## Not Doing (Anti-Goals)

- ❌ Multi-band compressor
- ❌ Sidechain / external routing
- ❌ Graphical EQ curve display
- ❌ Advanced reverb controls (decay, pre-delay, diffusion)
- ❌ Saturation model switching in UI (keep one knob)
- ❌ Expanding SPACE beyond 3 modes
- ❌ Adding tabs or advanced pages

## First Step Recommendation

**Stage 1 first** — it's low risk, structural, and fixes things that are actually broken (case 3 room reverb violates design).
