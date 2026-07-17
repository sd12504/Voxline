# VOXLINE Core DSP Modules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 以可獨立測試的模組取代 Processor 內的單體 DSP，完成正確電表、EQ、De-ess、Comp、Polish、Drive、Space、Output Safety 與正式訊號鏈。

**Architecture:** 每個 DSP 模組只接受 plain settings，不直接讀 APVTS，也不觸碰 UI。Meter/Output、VocalEq、DeEsser、VocalCompressor、VocalPolish、VocalDrive、VocalSpace Agent 分別只擁有同名的 `Source/DSP/` 與 `Tests/` 檔案；`PluginProcessor.*` 僅在所有模組通過後由單一 integration owner 修改。

**Tech Stack:** C++20、JUCE 8.0.12、`juce::dsp`、CMake 3.22+、JUCE UnitTest／CTest。

## Global Constraints

- 訊號流程固定為 `Input Gain → Input Meter → Vocal EQ → DE-ESS → COMP → POLISH → DRIVE → SPACE → Output Gain → Emergency Soft Clip → Output Meter`。
- 目標總延遲不高於約 1.5 ms，並以 `setLatencySamples()` 正確回報。
- 支援 44.1、48、88.2、96 kHz；mono 與 stereo；block size 64、512、2048。
- audio thread 不配置記憶體、不做檔案 I/O、不持有 UI lock。
- 所有模式、參數、A/B、Preset 與 Bypass 變化必須平滑，不能產生 click、NaN 或 Inf。
- POLISH 不得改寫或縮放 EQ、DE-ESS、COMP、DRIVE、SPACE、Input Gain 或 Output Gain。
- BODY／PRESENCE／AIR 與 Advanced LOW／PRES／AIR 共用實際參數；LOW、MUD、PRES、AIR 的 gain 為 −12.0…+12.0 dB、step 0.1、default 0。
- 本計畫不修改 UI、User Preset 檔案管理或 Host state migration。
- 不修改或提交 `.superpowers/`、`design-qa.md`、`tools/`、`voxline-editable-figma.svg`。

---

## Shared Interfaces

`Source/DSP/DspTypes.h`：

```cpp
#pragma once

#include <JuceHeader.h>

namespace Voxline::Dsp
{
inline constexpr float silenceFloorDbfs = -60.0f;

struct ModuleSpec
{
    double sampleRate {44100.0};
    int maximumBlockSize {512};
    int channels {2};
};

float gainToDbfs(float linear) noexcept;
float smoothingCoefficient(double sampleRate, float timeMs) noexcept;
}
```

所有音訊模組使用相同 lifecycle：

```cpp
void prepare(const Voxline::Dsp::ModuleSpec&);
void reset() noexcept;
void setTargetSettings(const Settings&) noexcept;
void process(juce::AudioBuffer<float>&) noexcept;
```

`prepare()` 是唯一允許配置 buffer／oversampling／delay memory 的位置。

---

### Task 1: Shared DSP types and numerical helpers

**Files:**
- Create: `Source/DSP/DspTypes.h`
- Create: `Source/DSP/DspTypes.cpp`
- Create: `Tests/DspTypesTests.cpp`

**Produces:** `ModuleSpec`, `gainToDbfs()`, `smoothingCoefficient()`

- [ ] **Step 1: Write failing helper tests**

```cpp
#include <JuceHeader.h>
#include "../Source/DSP/DspTypes.h"

namespace
{
class DspTypesTests final : public juce::UnitTest
{
public:
    DspTypesTests() : juce::UnitTest("DSP types", "VOXLINE") {}

    void runTest() override
    {
        beginTest("linear gain converts to bounded dBFS");
        expectWithinAbsoluteError(Voxline::Dsp::gainToDbfs(0.5f),
                                  -6.0206f, 0.001f);
        expectEquals(Voxline::Dsp::gainToDbfs(0.0f),
                     Voxline::Dsp::silenceFloorDbfs);

        beginTest("smoothing coefficient represents the requested sample time");
        const auto coefficient =
            Voxline::Dsp::smoothingCoefficient(48000.0, 100.0f);
        expectWithinAbsoluteError(std::pow(coefficient, 4800.0f),
                                  std::exp(-1.0f), 1.0e-4f);
    }
};

DspTypesTests dspTypesTests;
}
```

- [ ] **Step 2: Verify RED**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
```

Expected: FAIL because `Source/DSP/DspTypes.h` does not exist.

- [ ] **Step 3: Implement helpers**

```cpp
#include "DspTypes.h"

float Voxline::Dsp::gainToDbfs(float linear) noexcept
{
    return juce::Decibels::gainToDecibels(
        juce::jmax(0.0f, linear), silenceFloorDbfs);
}

float Voxline::Dsp::smoothingCoefficient(double sampleRate,
                                         float timeMs) noexcept
{
    const auto samples = static_cast<float>(
        juce::jmax(1.0, sampleRate * timeMs * 0.001));
    return std::exp(-1.0f / samples);
}
```

- [ ] **Step 4: Verify GREEN and commit**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
ctest --test-dir build -C Release --output-on-failure
git add Source/DSP/DspTypes.h Source/DSP/DspTypes.cpp Tests/DspTypesTests.cpp
git commit -m "feat: add shared VOXLINE DSP types"
```

Expected: build succeeds and CTest 1/1 passes.

---

### Task 2: Metering and continuous output safety

**Files:**
- Create: `Source/DSP/Metering.h`
- Create: `Source/DSP/Metering.cpp`
- Create: `Source/DSP/OutputSafety.h`
- Create: `Source/DSP/OutputSafety.cpp`
- Create: `Tests/MeterOutputTests.cpp`

**Consumes:** `DspTypes.h`

**Produces:**

```cpp
namespace Voxline::Dsp
{
struct ChannelMeter
{
    float peakDbfs {silenceFloorDbfs};
    float rmsDbfs {silenceFloorDbfs};
    float truePeakDbtp {silenceFloorDbfs};
};

struct MeterFrame
{
    std::array<ChannelMeter, 2> channels;
    int channelCount {};
    bool clipHeld {};
};

class BallisticMeter
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    MeterFrame measureBlock(const juce::AudioBuffer<float>&) noexcept;
    void clearClipHold() noexcept;
};

class EmergencySoftClipper
{
public:
    void reset() noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    bool wasActive() const noexcept;
    static float transfer(float sample) noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

Tests must assert:

- 0.5-amplitude sine: Peak −6.0206 dBFS and RMS −9.0309 dBFS within 0.05 dB after settling.
- Silence: all values −60 dBFS, no clip hold.
- Same 400 ms release elapsed through 64- and 512-sample blocks differs by less than 0.1 dB.
- Inter-sample stress waveform reports True Peak greater than sample Peak.
- Any sample at or above 0 dBFS sets clip hold; `clearClipHold()` clears it.
- `transfer(0.98−1e-5)` and `transfer(0.98+1e-5)` differ continuously; finite-difference slopes on each side differ by less than 0.02.
- Extreme ±100 input stays finite and bounded within ±1.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `Metering.h`／`OutputSafety.h`.

- [ ] **Step 3: Implement sample-time ballistics**

Use peak attack 8 ms, release 400 ms; RMS attack 80 ms, release 600 ms. Update per sample or use block-duration exponentiation:

```cpp
const auto blockCoefficient =
    std::pow(perSampleCoefficient, static_cast<float>(numSamples));
state = target + blockCoefficient * (state - target);
```

Store and expose dBFS/dBTP only; do not normalise to 0…1. True Peak uses preallocated 4× oversampling/interpolation state.

- [ ] **Step 4: Implement continuous soft clip**

For magnitude `a`, threshold `t=0.98`:

```cpp
if (a <= t)
    return sample;

const auto normalised = (a - t) / (1.0f - t);
const auto curved = t + (1.0f - t) * (1.0f - std::exp(-normalised));
return std::copysign(juce::jmin(curved, 1.0f), sample);
```

Blend through a 0.02-wide cubic transition if the raw exponential slopes do not meet the C1 test.

- [ ] **Step 5: Verify GREEN and commit**

Run focused suite, then CTest. Commit only the four module files and `MeterOutputTests.cpp`:

```bash
git commit -m "feat: add accurate metering and output safety"
```

---

### Task 3: Six-band Vocal EQ

**Files:**
- Create: `Source/DSP/VocalEq.h`
- Create: `Source/DSP/VocalEq.cpp`
- Create: `Tests/VocalEqTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
enum class FilterSlope { db12, db24, db36, db48 };

struct EqBandSettings
{
    bool enabled {};
    float frequencyHz {};
    float gainDb {};
    float q {1.0f};
};

struct VocalEqSettings
{
    bool enabled {true};
    EqBandSettings hpf {false, 80.0f, 0.0f, 0.707f};
    EqBandSettings low {true, 160.0f, 0.0f, 0.8f};
    EqBandSettings mud {false, 350.0f, 0.0f, 1.1f};
    EqBandSettings presence {true, 2500.0f, 0.0f, 1.0f};
    EqBandSettings air {true, 10000.0f, 0.0f, 0.7f};
    EqBandSettings lpf {false, 18000.0f, 0.0f, 0.707f};
    FilterSlope hpfSlope {FilterSlope::db24};
    FilterSlope lpfSlope {FilterSlope::db12};
};

class VocalEq
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const VocalEqSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    float magnitudeAt(float frequencyHz) const noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- All bands default gain 0 and HPF/MUD/LPF disabled: processed impulse matches input within `1e-5`.
- Global EQ disabled: exact null.
- LOW/MUD/PRES/AIR at ±12 dB produce centre response within ±0.75 dB.
- HPF and LPF disabled are true null; each enabled slope has monotonically increasing attenuation.
- 44.1/48/88.2/96 kHz centres remain within 2% and all frequencies clamp below `0.45 * sampleRate`.
- A gain/frequency/Q automation step produces only finite samples and maximum adjacent delta below 0.5 for a 0.2 sine input.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `VocalEq.h`.

- [ ] **Step 3: Implement**

Use per-channel filter chains and current/target coefficient crossfade over 10 ms. Use actual sample rate for every coefficient and `juce::jlimit(20.0f, 0.45f * sampleRate, frequency)`. LOW is a low shelf, MUD/PRES are peaks, AIR is a high shelf. Disabled bands are bypassed without hidden coefficients.

- [ ] **Step 4: Verify and commit**

Run focused VocalEq tests at all sample rates, then full CTest. Commit:

```bash
git commit -m "feat: add independent six-band vocal EQ"
```

---

### Task 4: Linked stereo De-esser

**Files:**
- Create: `Source/DSP/DeEsser.h`
- Create: `Source/DSP/DeEsser.cpp`
- Create: `Tests/DeEsserTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
enum class DeEssMode { split, wide };

struct DeEsserSettings
{
    float amount {};
    float focusHz {6500.0f};
    float sensitivityDb {-18.0f};
    float maxRangeDb {6.0f};
    DeEssMode mode {DeEssMode::split};
};

struct DeEssMetrics
{
    float reductionDb {};
    bool sActive {};
};

class DeEsser
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const DeEsserSettings&) noexcept;
    DeEssMetrics process(juce::AudioBuffer<float>&) noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- Amount 0 is sample-exact null.
- Split reduces 8 kHz by at least 3 dB while changing simultaneous 1 kHz by less than 0.75 dB.
- Wide reduces both components when sibilance triggers.
- Sibilance in only left input applies equal linked gain reduction to both channels.
- Max Range is never exceeded; >9 dB is possible only when user range permits it.
- Attack/release elapsed time differs by less than 2 ms across sample rates/block sizes.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `DeEsser.h`.

- [ ] **Step 3: Implement**

Use linked max/RMS detector over a focus band, sample-time attack/release, and preallocated split-band filtering. `amount` maps detector sensitivity and target reduction but never reads Polish. Return actual reduction from the applied envelope.

- [ ] **Step 4: Verify and commit**

Commit:

```bash
git commit -m "feat: add linked stereo de-esser"
```

---

### Task 5: Target-level Vocal Compressor

**Files:**
- Create: `Source/DSP/VocalCompressor.h`
- Create: `Source/DSP/VocalCompressor.cpp`
- Create: `Tests/VocalCompressorTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
struct CompressorSettings
{
    float amount {};
    float sensitivity {};
    float ratio {3.0f};
    float attackMs {15.0f};
    float releaseMs {80.0f};
    float mix {1.0f};
    float makeupDb {};
    bool autoMakeup {true};
};

struct CompressorMetrics { float gainReductionDb {}; };

class VocalCompressor
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const CompressorSettings&) noexcept;
    CompressorMetrics process(juce::AudioBuffer<float>&) noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- Amount 0 and Mix 0 are null.
- Calibrated −12 dBFS vocal-like signal gives target steady GR:
  - 25 → 1–2 dB.
  - 50 → 3–4 dB.
  - 75 → 5–7 dB.
  - 100 → 8–10 dB.
- Attack/release timings stay within 2 ms across 48/96 kHz.
- Linked stereo produces left/right gain mismatch under 0.05 dB.
- Auto Makeup keeps long-term output RMS within 0.5 dB of input without exceeding 0 dBFS in the test signal.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `VocalCompressor.h`.

- [ ] **Step 3: Implement**

Use linked RMS detector and soft knee. Map Amount monotonically to an internal target reduction; Sensitivity offsets the detector reference rather than exposing raw Threshold. Apply dry/wet Mix and a slow average auto-makeup estimator capped at +10 dB.

- [ ] **Step 4: Verify and commit**

```bash
git commit -m "feat: add target-level vocal compressor"
```

---

### Task 6: Independent one-knob Polish

**Files:**
- Create: `Source/DSP/VocalPolish.h`
- Create: `Source/DSP/VocalPolish.cpp`
- Create: `Tests/VocalPolishTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
struct PolishSettings { float amount {}; };

class VocalPolish
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const PolishSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- 0% is sample-for-sample null.
- Changing Polish never mutates any external settings object or APVTS parameter.
- 50% increases crest-density/presence measurably but long-term RMS stays within 0.75 dB.
- 100% stays finite and does not exceed ±1.2 before Output Safety.
- 44.1–96 kHz responses remain consistent within 1 dB.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `VocalPolish.h`.

- [ ] **Step 3: Implement**

Implement only internal 10 ms smoothed amount, light upward/downward dynamic shaping, low-order warmth saturation, subtle presence/air shelves, and slow internal loudness compensation. Public API exposes only Amount; no references to other module settings or parameters.

- [ ] **Step 4: Verify and commit**

```bash
git commit -m "feat: add independent one-knob polish"
```

---

### Task 7: Level-matched Vocal Drive

**Files:**
- Create: `Source/DSP/VocalDrive.h`
- Create: `Source/DSP/VocalDrive.cpp`
- Create: `Tests/VocalDriveTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
enum class DriveCharacter { clean, warm, edge };

struct DriveSettings
{
    float amount {};
    DriveCharacter character {DriveCharacter::warm};
    float tone {};
    float mix {0.7f};
    float outputTrimDb {};
    bool levelMatch {true};
};

class VocalDrive
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const DriveSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    int latencySamples() const noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- Amount 0 is exact null.
- Each character produces distinct harmonic ratios.
- Level Match keeps 1-second RMS within 0.5 dB; disabled mode permits level change.
- Edge at maximum remains finite.
- 7 kHz sine alias-band energy is at least 30 dB below the fundamental.
- Reported latency equals measured impulse delay and fits total latency budget.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `VocalDrive.h`.

- [ ] **Step 3: Implement**

Use preallocated 2× or 4× JUCE oversampling, mode-specific asymmetric transfer curves, smoothed pre-gain/Tone/Mix/Trim, and a slow capped level-match estimator. No lookahead.

- [ ] **Step 4: Verify and commit**

```bash
git commit -m "feat: add level-matched vocal drive"
```

---

### Task 8: Five-mode Vocal Space

**Files:**
- Create: `Source/DSP/VocalSpace.h`
- Create: `Source/DSP/VocalSpace.cpp`
- Create: `Tests/VocalSpaceTests.cpp`

**Produces:**

```cpp
namespace Voxline::Dsp
{
enum class SpaceMode { room, plate, hall, slap, width };

struct SpaceSettings
{
    float amount {};
    SpaceMode mode {SpaceMode::plate};
    float preDelayMs {28.0f};
    float sizeOrTime {0.62f};
    float decaySeconds {1.6f};
    float tone {0.12f};
    float width {1.0f};
    float ducking {0.42f};
    float feedback {0.2f};
    bool monoSafety {true};
};

class VocalSpace
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const SpaceSettings&) noexcept;
    void process(juce::AudioBuffer<float>& audio,
                 const juce::AudioBuffer<float>& drySidechain) noexcept;
    double tailSeconds() const noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- Amount 0 is exact null.
- Pre-delay moves first wet sample to requested time ±1 sample.
- Room tail is shortest, Hall longest; Plate has denser early reflections than Room.
- Slap first repeat equals Time ±1 sample and feedback decays.
- Width increases stereo difference but mono fold-down stays within 1 dB when Mono Safety is on.
- Ducking reduces wet RMS during voice and releases afterward.
- Mode switch maximum adjacent delta remains under 0.5 for a 0.2 sine input.
- `prepare()` allocates all buffers; repeated process does not resize them.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `VocalSpace.h`.

- [ ] **Step 3: Implement**

Use separate preallocated engines:

- Room: short diffuser + feedback delay network.
- Plate: four all-pass diffusers + modulated feedback tank.
- Hall: longer eight-line FDN with low-rate modulation.
- Slap: stereo delay with feedback/tone.
- Width: micro-delay/spread with mono-safe mid/side limit.

Keep current and next engines alive during a 30 ms mode crossfade. Duck wet signal from linked dry-sidechain envelope.

- [ ] **Step 4: Verify and commit**

```bash
git commit -m "feat: add five-mode vocal space"
```

---

### Task 9: Serial Processor integration

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`
- Create: `Tests/ProcessorIntegrationTests.cpp`

**Consumes:** all Tasks 1–8 module APIs.

- [ ] **Step 1: Write failing integration tests**

Tests must cover:

- Input Meter reads post-Input Gain/pre-EQ.
- Output Meter reads post-Output Gain/soft clip/bypass crossfade.
- Impulse/differential checks establish exact module order.
- Polish 0 is null and changing Polish leaves all other APVTS values unchanged.
- Removed Auto Gain, Clean Mode and global Listen values cannot affect output.
- Bypass settles to exact dry after its crossfade.
- 44.1/48/88.2/96 kHz × mono/stereo × 64/512/2048 remain finite.
- Processor reports exactly `VocalDrive::latencySamples()`.
- Process calls do not resize dry/module buffers.

- [ ] **Step 2: Verify RED**

Expected: current Processor fails meter unit/order, Polish independence, removed-function no-op and allocation assertions.

- [ ] **Step 3: Replace monolithic DSP with module members**

Processor header owns one instance of each module plus preallocated dry/sidechain buffers and atomic meter frames. `prepareToPlay()` prepares every module and buffer; `processBlock()` snapshots parameters, calls modules in specified order, performs bypass crossfade, and publishes actual dB/dBTP/GR metrics.

Delete old filter arrays, envelopes, delay state, `polishScale`, wetMix, Auto Gain/Clean Mode/Listen audio branches, and discontinuous `applySoftClip()`.

- [ ] **Step 4: Verify GREEN**

Run focused integration tests, full CTest, Release VST3/AU builds, and a no-allocation debug probe.

- [ ] **Step 5: Commit**

```bash
git add Source/PluginProcessor.h Source/PluginProcessor.cpp \
  Tests/ProcessorIntegrationTests.cpp
git commit -m "refactor: integrate modular VOXLINE signal chain"
```

---

### Task 10: Core DSP final verification

- [ ] Run every DSP test suite at 44.1/48/88.2/96 kHz and mono/stereo.
- [ ] Run CTest, Release VST3, Release AU.
- [ ] Confirm `rg "polishScale|cleanModeCoeff|autoGainCompensation|applySoftClip" Source/PluginProcessor.*` returns no active DSP path.
- [ ] Confirm no module reads APVTS or includes `PluginProcessor.h`.
- [ ] Generate a whole-plan diff package and obtain final code review.
- [ ] Record any Minor findings for final integration; fix every Critical/Important finding before UI integration.

## Parallel Execution Batches

1. Task 1 serial.
2. Tasks 2–4 parallel.
3. Tasks 5–8 parallel in available slots.
4. Task 9 serial integration.
5. Task 10 verification and review.
