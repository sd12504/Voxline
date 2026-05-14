# AGENT.md — VOXLINE Cross-Platform Plugin Development Guide

## 0. Project Decision

VOXLINE is being rebuilt from the ground up.

This is not a UI-only rewrite.  
This is a clean restart of the full plugin project: project structure, parameters, UI, DSP, testing, and packaging.

The goal is to avoid carrying over broken UI interaction, broken knob bindings, unclear architecture, or unstable DSP from the old version.

---

## 1. Project Identity

Project name: **VOXLINE**

Product type: **Fast Vocal Channel Strip Plugin**

Core idea:

> A modern, beautiful, creator-friendly vocal plugin that makes demo vocals sound polished quickly.

Target users:

- Singers
- Rappers
- Creators
- Demo producers
- Bedroom producers
- Vocal editors
- Music makers who want fast vocal processing

Core workflow:

```txt
Choose preset → Turn POLISH → Fine-tune a few controls → Done
```

VOXLINE is not:

- A DAW
- A pitch correction plugin
- A reverb plugin
- A delay plugin
- A complex analog emulation project
- An AI cloud mixing product

---

## 2. Required Tech Stack

Use this stack unless explicitly instructed otherwise:

```txt
Framework: JUCE
Language: C++20
Build system: CMake
Platforms: Windows + macOS
Windows format: VST3
macOS formats: VST3 + AU
Primary development platform: macOS is acceptable, but code must remain Windows-compatible
Version control: Git
```

Recommended IDEs:

```txt
macOS: Xcode / CLion / VS Code
Windows: Visual Studio 2022 / CLion / VS Code
```

Do not switch to another framework without asking.

Do not use Swift, Electron, WebAudio, HISE, iPlug2, Max/MSP, or plugdata for the main commercial plugin.

plugdata may be used only as a separate DSP sketch/prototype tool, not as the main plugin implementation.

---

## 3. Cross-Platform Rules

The project must support both Windows and macOS from the start.

Rules:

```txt
1. Do not write macOS-only code unless guarded with JUCE_MAC.
2. Do not write Windows-only code unless guarded with JUCE_WINDOWS.
3. Do not hardcode absolute local paths.
4. Do not assume '/' or '\\' path separators manually; use juce::File or CMake paths.
5. Do not depend on fonts that only exist on one platform.
6. Do not depend on Xcode-only behavior.
7. Do not depend on Visual Studio-only behavior.
8. Use CMake as the source of truth.
9. Keep DSP and UI code platform-neutral whenever possible.
10. Every phase should keep the project buildable on at least the current platform and should not knowingly break the other platform.
```

Plugin formats:

```txt
Windows: VST3
macOS: VST3, AU
```

Do not add AAX for MVP.

---

## 4. Development Principle

Build gradually.

Do not attempt to build the full plugin at once.

The correct development order is:

```txt
Phase 0: Clean project initialization
Phase 1: APVTS parameter foundation
Phase 2: Functional debug UI
Phase 3: Minimal vocal DSP chain
Phase 4: Fixed Figma layout
Phase 5: Custom knob system
Phase 6: Light / Dark theme system
Phase 7: Meters, presets, and visual polish
Phase 8: Cross-platform verification and packaging
```

Important rule:

> First make it build. Then make it move. Then make it sound. Then make it beautiful.

Do not start with final Figma UI.  
Do not start with custom knobs.  
Do not start with complex DSP.

---

## 5. Git Rules

Initialize Git before development.

Every completed phase must end with one commit.

Before starting work:

```bash
git status
```

After completing a phase:

```bash
git add .
git commit -m "<clear commit message>"
```

Commit only when:

- The code builds
- The plugin opens if the phase requires opening it
- The current phase works
- No obvious crash exists
- TODO / README are updated when relevant

Do not make one giant commit at the end.

Suggested commit style:

```bash
git commit -m "chore: initialize cross-platform JUCE plugin project"
git commit -m "feat: add APVTS parameter foundation"
git commit -m "ui: add functional debug controls"
git commit -m "feat: add minimal vocal DSP chain"
git commit -m "ui: implement fixed Figma layout"
git commit -m "ui: add custom knob system"
git commit -m "ui: add light and dark themes"
git commit -m "feat: add meters presets and visual polish"
git commit -m "test: verify Windows and macOS plugin builds"
```

---

## 6. Communication Rules for Agent

Before changing code, always report:

```txt
Plan:
- What phase this is
- Which files will be changed
- What each file is responsible for
- What this change is meant to fix or create
- How the user should test it
```

After completing work, always report:

```txt
Completed:
- ...

Changed files:
- ...

Tested:
- ...

Commit:
- ...

Next step:
- ...
```

If blocked, report:

```txt
Problem:
- ...

Likely cause:
- ...

Suggested fix:
- ...
```

The user is not a JUCE expert. Explain changes in practical terms.

---

## 7. MVP Controls

MVP controls:

| Parameter ID | UI Label | Function |
|---|---|---|
| `inputGain` | GAIN | Input trim |
| `autoGain` | AUTO GAIN | Output compensation helper |
| `polish` | POLISH | Global vocal polish macro |
| `body` | BODY | Vocal thickness |
| `clarity` | CLARITY | Vocal forwardness / intelligibility |
| `air` | AIR | High-frequency shine |
| `smooth` | SMOOTH | De-esser / harshness reduction |
| `comp` | COMP | One-knob compression amount |
| `drive` | DRIVE | Saturation / color |
| `outputGain` | OUT | Final output gain |
| `bypass` | BYPASS | Bypass processing |
| `listen` | LISTEN | Future monitoring / listen mode |

Do not use inconsistent names like `out`, `output`, `polishAmount`, or `input_gain`.

Parameter IDs must remain stable once created.

---

## 8. APVTS Rules

Use `juce::AudioProcessorValueTreeState` as the parameter source of truth.

APVTS must support:

- UI control binding
- DAW automation
- Preset/state saving
- State restore after reopening plugin
- Processor-side parameter reading

Rules:

```txt
1. All plugin parameters must be defined in one clear parameter layout function.
2. SliderAttachment / ButtonAttachment objects must be stored as PluginEditor members.
3. Do not create attachments as constructor-local variables.
4. Parameter IDs must exactly match UI attachment IDs.
5. Parameters must have safe ranges and default values.
```

Most important interaction chain:

```txt
User moves control
→ Slider/Button changes
→ Attachment writes APVTS parameter
→ Processor reads parameter
→ DSP changes sound
→ UI repaints / meter updates
```

If a knob does not react, check this chain first.

---

## 9. DSP Direction

Initial DSP chain:

```txt
Input Gain
→ Body EQ
→ Clarity EQ
→ Air EQ
→ Smooth / De-esser placeholder
→ Compressor placeholder
→ Drive saturation
→ Output Gain
→ Safety soft clip / protection
→ Metering
```

Use safe, simple DSP first.

Do not attempt advanced analog modeling in version 1.

### 9.1 Gain

- Use smoothed gain changes
- Avoid zipper noise
- Avoid sudden jumps

### 9.2 Body / Clarity / Air

Suggested mapping:

```txt
Body: low-mid bell or shelf around 120 Hz - 300 Hz
Clarity: bell around 2 kHz - 5 kHz
Air: high shelf around 10 kHz - 16 kHz
```

Keep gain range moderate:

```txt
-6 dB to +6 dB
```

### 9.3 Smooth

Initial version can be a safe placeholder:

```txt
High-frequency harshness reduction / simple de-esser-like behavior
```

Do not build complex AI de-essing in MVP.

### 9.4 Comp

Expose only one control:

```txt
Comp: 0 - 100
```

Internally map to threshold / ratio / attack / release / makeup.

### 9.5 Drive

Initial version:

```txt
tanh saturation or soft clip saturation
```

Drive 0 should be transparent.

### 9.6 Polish Macro

POLISH should gently increase:

- Compression
- Smooth
- Subtle EQ polish
- Subtle saturation
- Output compensation

POLISH must not feel random.

---

## 10. UI Direction

The final UI should follow `UI.md`, but only after the functional debug UI works.

Do not begin with the final Figma UI.

UI implementation order:

```txt
1. Functional debug UI using default sliders/buttons
2. Fixed Figma layout using real controls
3. Custom knob system
4. Theme system
5. Meters / presets / polish pass
```

Important:

```txt
CustomKnob must be a real interactive control.
Preferred: inherit juce::Slider and custom-paint it.
Do not draw fake knobs as passive Components.
```

The final UI must not use:

- A screenshot of the full UI
- React / HTML / CSS
- Static fake controls
- PNG knobs for dynamic controls
- Default gray JUCE styling as the final look

---

## 11. Required Source Structure

Suggested structure:

```txt
VOXLINE/
  CMakeLists.txt
  README.md
  AGENT.md
  TODO.md
  UI.md

  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp

    Parameters/
      ParameterIDs.h
      ParameterLayout.h
      ParameterLayout.cpp

    DSP/
      VocalChain.h
      VocalChain.cpp
      SimpleToneEQ.h
      SimpleToneEQ.cpp
      SimpleCompressor.h
      SimpleCompressor.cpp
      SimpleDeEsser.h
      SimpleDeEsser.cpp
      Saturation.h
      Saturation.cpp

    UI/
      Layout.h
      Theme.h
      Theme.cpp
      CustomKnob.h
      CustomKnob.cpp
      HeroKnob.h
      HeroKnob.cpp
      SectionPanel.h
      SectionPanel.cpp
      LevelMeter.h
      LevelMeter.cpp
      PillButton.h
      PillButton.cpp
      ToggleSwitch.h
      ToggleSwitch.cpp
```

Do not over-engineer before the MVP works.

---

## 12. Presets

MVP presets:

```txt
Clean
Warm
Bright
Rap
```

Preset behavior:

- Clean: natural and transparent
- Warm: more body and drive
- Bright: more clarity and air, with smoothing
- Rap: more compression, forwardness, and saturation

Preset buttons can initially set parameter values directly.

A full preset browser can wait.

---

## 13. Testing Requirements

Minimum macOS tests:

- Build VST3
- Build AU
- Open in JUCE AudioPluginHost
- Open in Reaper if available
- Open in Logic Pro if available
- Run AU validation when relevant

Minimum Windows tests:

- Build VST3
- Open in a VST3 host such as Reaper or JUCE AudioPluginHost
- Confirm automation visibility
- Confirm plugin state restore

General tests:

- Plugin opens
- Audio passes through
- Bypass works
- Controls move and update parameters
- Parameters automate
- Presets switch safely
- No crash when opening/closing plugin
- No loud pop when changing values
- Multiple instances do not crash
- Sample rates: 44.1 / 48 / 96 kHz
- Buffer sizes: 64 / 128 / 256 / 512

---

## 14. Things Not To Do

Do not:

- Rewrite the whole architecture every time after Phase 0
- Start with final Figma UI before controls work
- Create fake knobs that do not move
- Create APVTS attachments as local variables
- Add reverb in MVP
- Add delay in MVP
- Add pitch correction in MVP
- Add AI features in MVP
- Add account login
- Add online license checking
- Add AAX support
- Add heavy animation before audio is stable
- Use copyrighted logos or copied UI assets
- Copy another plugin’s interface directly
- Hardcode platform-specific paths

---

## 15. Definition of Done

A phase is done only when:

```txt
The code builds
The plugin opens if applicable
The feature works
The UI does not break
The TODO is updated
A Git commit is created
```

Final MVP is done when:

```txt
Windows VST3 builds
macOS VST3 builds
macOS AU builds
Plugin opens in at least one host per platform target
All MVP controls work
Audio processing is stable
No obvious crashes
UI is usable and resembles the Figma direction
README explains build and test steps
```
