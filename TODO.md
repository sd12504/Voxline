# TODO.md — VOXLINE Clean Rebuild Roadmap

## Rule

Every completed phase must end with one Git commit.

Before starting a phase:

```bash
git status
```

After completing a phase:

```bash
git add .
git commit -m "<phase commit message>"
```

Do not skip phases.  
Do not jump directly to final UI.  
Do not build fake knobs.

---

# Phase 0 — Clean Cross-Platform Project Initialization

## Goal

Create a clean JUCE plugin project for Windows + macOS.

## Platform Targets

```txt
Windows: VST3
macOS: VST3 + AU
```

## Tasks

- [x] Create clean `VOXLINE` project folder
- [x] Initialize Git
- [x] Add `AGENT.md`
- [x] Add `TODO.md`
- [x] Add `UI.md`
- [x] Add `README.md`
- [x] Add JUCE as submodule or clearly documented dependency
- [x] Create root `CMakeLists.txt`
- [x] Configure C++20
- [x] Configure plugin name as `VOXLINE`
- [x] Configure manufacturer as `ONETAKE`
- [x] Enable VST3 for Windows and macOS
- [x] Enable AU for macOS
- [x] Create minimal source files:
  - [x] `Source/PluginProcessor.h`
  - [x] `Source/PluginProcessor.cpp`
  - [x] `Source/PluginEditor.h`
  - [x] `Source/PluginEditor.cpp`
- [x] Confirm macOS build path works
- [x] Ensure no macOS-only paths are hardcoded
- [x] Ensure Windows build path is planned and documented

## Suggested Commit

```bash
git add .
git commit -m "chore: initialize clean cross-platform VOXLINE JUCE project"
```

## Done When

- [x] Git repository exists
- [x] CMake project exists
- [x] Minimal plugin builds on current machine
- [x] README has macOS and Windows build commands
- [x] No platform-specific path assumptions exist

---

# Phase 1 — APVTS Parameter Foundation

## Goal

Create the full stable parameter system before UI and DSP become complex.

## Required Parameters

- [x] `inputGain`
- [x] `autoGain`
- [x] `polish`
- [x] `body`
- [x] `clarity`
- [x] `air`
- [x] `smooth`
- [x] `comp`
- [x] `drive`
- [x] `outputGain`
- [x] `bypass`
- [x] `listen`

## Parameter Ranges

```txt
inputGain:  -24 dB to +24 dB, default 0 dB
outputGain: -24 dB to +24 dB, default 0 dB

polish:     0% to 100%, default 65%
body:       0% to 100%, default 54%
clarity:    0% to 100%, default 61%
air:        0% to 100%, default 48%
smooth:     0% to 100%, default 32%
comp:       0% to 100%, default 42%
drive:      0% to 100%, default 18%

autoGain:   boolean, default on
bypass:     boolean, default off
listen:     boolean, default off
```

## Tasks

- [x] Implement `juce::AudioProcessorValueTreeState`
- [x] Create centralized parameter ID definitions
- [x] Create parameter layout function
- [x] Add all required parameters
- [x] Add state save / restore
- [x] Confirm parameters appear in host automation
- [x] Confirm parameter values persist after closing and reopening plugin

## Suggested Commit

```bash
git add .
git commit -m "feat: add VOXLINE APVTS parameter foundation"
```

## Done When

- [x] Parameters exist
- [x] Host automation can see parameters
- [x] State save / load works
- [x] No crash

---

# Phase 2 — Functional Debug UI

## Goal

Create an ugly but completely functional UI to verify all controls work.

Do not make final Figma UI yet.

## Tasks

- [x] Set editor size to `1100 x 760`
- [x] Add default JUCE sliders for:
  - [x] `inputGain`
  - [x] `polish`
  - [x] `body`
  - [x] `clarity`
  - [x] `air`
  - [x] `smooth`
  - [x] `comp`
  - [x] `drive`
  - [x] `outputGain`
- [x] Add default JUCE toggle buttons for:
  - [x] `autoGain`
  - [x] `bypass`
  - [x] `listen`
- [x] Create SliderAttachments as `PluginEditor` members
- [x] Create ButtonAttachments as `PluginEditor` members
- [x] Confirm no attachment is a constructor-local variable
- [x] Add simple labels showing values if needed
- [x] Add debug logging for `polish` value changes

## Suggested Commit

```bash
git add .
git commit -m "ui: add functional debug controls for all APVTS parameters"
```

## Done When

- [x] Every slider moves
- [x] Every toggle works
- [x] APVTS values update
- [x] Host automation updates
- [x] State restores correctly
- [x] No knob/control is fake

---

# Phase 3 — Minimal Vocal DSP Chain

## Goal

Make the plugin audibly process vocals with safe simple DSP.

## DSP Chain

```txt
Input Gain
→ Body EQ
→ Clarity EQ
→ Air EQ
→ Smooth high-frequency reduction placeholder
→ Compression placeholder
→ Drive saturation
→ Output Gain
→ Safety soft clip / protection
```

## Tasks

- [x] Implement smoothed input gain
- [x] Implement smoothed output gain
- [x] Implement bypass behavior
- [x] Implement Body EQ placeholder
- [x] Implement Clarity EQ placeholder
- [x] Implement Air EQ placeholder
- [x] Implement Smooth placeholder
- [x] Implement simple one-knob compression placeholder
- [x] Implement tanh / soft clip Drive
- [x] Implement simple POLISH macro mapping
- [x] Add basic output protection
- [ ] Test with vocal audio

## Suggested Commit

```bash
git add .
git commit -m "feat: add minimal VOXLINE vocal DSP chain"
```

## Done When

- [x] Audio passes cleanly
- [x] Each major control creates audible change
- [x] Bypass works
- [x] No loud clipping at default values
- [x] Parameter changes do not click badly

---

# Phase 4 — Fixed Figma Layout

## Goal

Replace debug layout with VOXLINE fixed layout while keeping all controls functional.

## Tasks

- [x] Add `Source/UI/Layout.h`
- [x] Use fixed editor size `1100 x 760`
- [x] Add top bar
- [x] Add Input panel
- [x] Add Tone Controls panel
- [x] Add Polish hero panel
- [x] Add Output panel
- [x] Add Bottom Preset bar
- [x] Move existing working controls into fixed Figma positions
- [x] Keep all APVTS attachments working
- [x] Confirm no panel blocks mouse clicks

## Suggested Commit

```bash
git add .
git commit -m "ui: implement fixed VOXLINE Figma layout"
```

## Done When

- [x] UI uses fixed layout from `UI.md`
- [x] Controls still move
- [x] Parameter attachment still works
- [x] Main panels are not stretched columns
- [x] POLISH is visually central

---

# Phase 5 — Custom Knob System

## Goal

Create VOXLINE custom knobs without breaking interaction.

## Rules

```txt
CustomKnob must be a real interactive control.
Preferred: inherit juce::Slider.
Do not make fake passive Component knobs.
Do not use PNG knobs for dynamic controls.
```

## Tasks

- [x] Create `CustomKnob`
- [x] Create `HeroKnob` for POLISH
- [x] Custom paint knob body
- [x] Custom paint inactive arc
- [x] Custom paint active arc based on current value
- [x] Custom paint pointer
- [x] Custom paint value text and label
- [x] Keep Slider interaction working
- [x] Keep SliderAttachment working
- [x] Test POLISH first
- [x] Then apply to all knobs

## Suggested Commit

```bash
git add .
git commit -m "ui: add interactive VOXLINE custom knob system"
```

## Done When

- [x] POLISH knob moves
- [x] All small knobs move
- [x] Value text updates
- [x] Active arc updates
- [x] APVTS sync remains correct
- [x] Host automation remains correct

---

# Phase 6 — Light / Dark Theme System

## Goal

Add theme tokens and implement both official themes.

## Tasks

- [x] Add `Theme.h`
- [x] Add `Theme.cpp`
- [x] Define `VoxlineTheme`
- [x] Implement Light Theme
- [x] Implement Dark Theme
- [x] Add temporary theme switch via 'T' key
- [x] Remove random hardcoded colors from knobs and editor
- [x] Confirm text contrast in both themes

## Suggested Commit

```bash
git add .
git commit -m "ui: add VOXLINE light and dark theme system"
```

## Done When

- [x] Light theme works
- [x] Dark theme works
- [x] Components read from theme tokens
- [x] UI remains readable

---

# Phase 7 — Meters, Presets, and Visual Polish

## Goal

Add the product-level features needed for the MVP UI.

## Tasks

- [x] Add output meter
- [x] Add gain reduction meter
- [x] Add PEAK readout
- [x] Add RMS readout placeholder or real value
- [x] Add preset buttons:
  - [x] Clean
  - [x] Warm
  - [x] Bright
  - [x] Rap
- [x] Add A/B button with snapshot toggle
- [x] Add Listen button behavior or placeholder
- [x] Add Settings placeholder
- [x] Add refined panel styling
- [x] Add button active states
- [x] Add final Figma spacing polish

## Suggested Commit

```bash
git add .
git commit -m "feat: add VOXLINE meters presets and visual polish"
```

## Done When

- [x] Meters respond or show stable placeholders
- [x] Preset buttons update parameters
- [x] No pops when switching presets
- [x] UI feels visually coherent
- [x] No obvious default JUCE gray controls remain

---

# Phase 8 — Cross-Platform Verification and Packaging

## Goal

Verify Windows + macOS plugin builds and prepare local install instructions.

## macOS Tasks

- [ ] Build Release VST3
- [ ] Build Release AU
- [ ] Install VST3 locally
- [ ] Install AU locally
- [ ] Test in JUCE AudioPluginHost
- [ ] Test in Logic Pro if available
- [ ] Test in Reaper if available
- [ ] Run AU validation if available

## Windows Tasks

- [ ] Build Release VST3
- [ ] Install VST3 locally
- [ ] Test in JUCE AudioPluginHost or Reaper
- [ ] Confirm automation visibility
- [ ] Confirm state restore

## General Tests

- [ ] Open / close plugin repeatedly
- [ ] Test multiple instances
- [ ] Test bypass
- [ ] Test preset switching
- [ ] Test 44.1 / 48 / 96 kHz
- [ ] Test buffer sizes 64 / 128 / 256 / 512
- [ ] Confirm no major crashes
- [ ] Update README with final build/test instructions

## Suggested Commit

```bash
git add .
git commit -m "test: verify VOXLINE Windows and macOS builds"
```

## Done When

- [ ] Windows VST3 builds
- [ ] macOS VST3 builds
- [ ] macOS AU builds
- [ ] Plugin opens in host
- [ ] README explains installation and testing

---

# Later Versions

Do not start before MVP is stable.

## Possible v1.1

- [ ] Better de-esser
- [ ] Oversampling for Drive
- [ ] More presets
- [ ] Resizable UI
- [ ] Preset browser
- [ ] Input meter
- [ ] Better auto gain

## Possible v1.2

- [ ] Doubler
- [ ] Simple delay
- [ ] Simple space ambience
- [ ] More skins
- [ ] A/B compare

## Possible v2

- [ ] AI preset suggestion
- [ ] Vocal assistant
- [ ] License system
- [ ] Installer
- [ ] AAX support
