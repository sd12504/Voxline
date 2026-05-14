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
- [ ] Add `AGENT.md`
- [ ] Add `TODO.md`
- [ ] Add `UI.md`
- [ ] Add `README.md`
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

- [ ] `inputGain`
- [ ] `autoGain`
- [ ] `polish`
- [ ] `body`
- [ ] `clarity`
- [ ] `air`
- [ ] `smooth`
- [ ] `comp`
- [ ] `drive`
- [ ] `outputGain`
- [ ] `bypass`
- [ ] `listen`

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

- [ ] Implement `juce::AudioProcessorValueTreeState`
- [ ] Create centralized parameter ID definitions
- [ ] Create parameter layout function
- [ ] Add all required parameters
- [ ] Add state save / restore
- [ ] Confirm parameters appear in host automation
- [ ] Confirm parameter values persist after closing and reopening plugin

## Suggested Commit

```bash
git add .
git commit -m "feat: add VOXLINE APVTS parameter foundation"
```

## Done When

- [ ] Parameters exist
- [ ] Host automation can see parameters
- [ ] State save / load works
- [ ] No crash

---

# Phase 2 — Functional Debug UI

## Goal

Create an ugly but completely functional UI to verify all controls work.

Do not make final Figma UI yet.

## Tasks

- [ ] Set editor size to `1100 x 760`
- [ ] Add default JUCE sliders for:
  - [ ] `inputGain`
  - [ ] `polish`
  - [ ] `body`
  - [ ] `clarity`
  - [ ] `air`
  - [ ] `smooth`
  - [ ] `comp`
  - [ ] `drive`
  - [ ] `outputGain`
- [ ] Add default JUCE toggle buttons for:
  - [ ] `autoGain`
  - [ ] `bypass`
  - [ ] `listen`
- [ ] Create SliderAttachments as `PluginEditor` members
- [ ] Create ButtonAttachments as `PluginEditor` members
- [ ] Confirm no attachment is a constructor-local variable
- [ ] Add simple labels showing values if needed
- [ ] Add debug logging for `polish` value changes

## Suggested Commit

```bash
git add .
git commit -m "ui: add functional debug controls for all APVTS parameters"
```

## Done When

- [ ] Every slider moves
- [ ] Every toggle works
- [ ] APVTS values update
- [ ] Host automation updates
- [ ] State restores correctly
- [ ] No knob/control is fake

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

- [ ] Implement smoothed input gain
- [ ] Implement smoothed output gain
- [ ] Implement bypass behavior
- [ ] Implement Body EQ placeholder
- [ ] Implement Clarity EQ placeholder
- [ ] Implement Air EQ placeholder
- [ ] Implement Smooth placeholder
- [ ] Implement simple one-knob compression placeholder
- [ ] Implement tanh / soft clip Drive
- [ ] Implement simple POLISH macro mapping
- [ ] Add basic output protection
- [ ] Test with vocal audio

## Suggested Commit

```bash
git add .
git commit -m "feat: add minimal VOXLINE vocal DSP chain"
```

## Done When

- [ ] Audio passes cleanly
- [ ] Each major control creates audible change
- [ ] Bypass works
- [ ] No loud clipping at default values
- [ ] Parameter changes do not click badly

---

# Phase 4 — Fixed Figma Layout

## Goal

Replace debug layout with VOXLINE fixed layout while keeping all controls functional.

## Tasks

- [ ] Add `Source/UI/Layout.h`
- [ ] Use fixed editor size `1100 x 760`
- [ ] Add top bar
- [ ] Add Input panel
- [ ] Add Tone Controls panel
- [ ] Add Polish hero panel
- [ ] Add Output panel
- [ ] Add Bottom Preset bar
- [ ] Move existing working controls into fixed Figma positions
- [ ] Keep all APVTS attachments working
- [ ] Confirm no panel blocks mouse clicks

## Suggested Commit

```bash
git add .
git commit -m "ui: implement fixed VOXLINE Figma layout"
```

## Done When

- [ ] UI uses fixed layout from `UI.md`
- [ ] Controls still move
- [ ] Parameter attachment still works
- [ ] Main panels are not stretched columns
- [ ] POLISH is visually central

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

- [ ] Create `CustomKnob`
- [ ] Create `HeroKnob` for POLISH
- [ ] Custom paint knob body
- [ ] Custom paint inactive arc
- [ ] Custom paint active arc based on current value
- [ ] Custom paint pointer
- [ ] Custom paint value text and label
- [ ] Keep Slider interaction working
- [ ] Keep SliderAttachment working
- [ ] Test POLISH first
- [ ] Then apply to all knobs

## Suggested Commit

```bash
git add .
git commit -m "ui: add interactive VOXLINE custom knob system"
```

## Done When

- [ ] POLISH knob moves
- [ ] All small knobs move
- [ ] Value text updates
- [ ] Active arc updates
- [ ] APVTS sync remains correct
- [ ] Host automation remains correct

---

# Phase 6 — Light / Dark Theme System

## Goal

Add theme tokens and implement both official themes.

## Tasks

- [ ] Add `Theme.h`
- [ ] Add `Theme.cpp`
- [ ] Define `ThemeColors`
- [ ] Implement Light Theme
- [ ] Implement Dark Theme
- [ ] Add temporary theme switch or debug flag
- [ ] Remove random hardcoded colors
- [ ] Confirm text contrast in both themes

## Suggested Commit

```bash
git add .
git commit -m "ui: add VOXLINE light and dark theme system"
```

## Done When

- [ ] Light theme works
- [ ] Dark theme works
- [ ] Components read from theme tokens
- [ ] UI remains readable

---

# Phase 7 — Meters, Presets, and Visual Polish

## Goal

Add the product-level features needed for the MVP UI.

## Tasks

- [ ] Add output meter
- [ ] Add gain reduction meter
- [ ] Add PEAK readout
- [ ] Add RMS readout placeholder or real value
- [ ] Add preset buttons:
  - [ ] Clean
  - [ ] Warm
  - [ ] Bright
  - [ ] Rap
- [ ] Add A/B button placeholder
- [ ] Add Listen button behavior or placeholder
- [ ] Add Settings placeholder
- [ ] Add refined panel styling
- [ ] Add button active states
- [ ] Add final Figma spacing polish

## Suggested Commit

```bash
git add .
git commit -m "feat: add VOXLINE meters presets and visual polish"
```

## Done When

- [ ] Meters respond or show stable placeholders
- [ ] Preset buttons update parameters
- [ ] No pops when switching presets
- [ ] UI feels visually coherent
- [ ] No obvious default JUCE gray controls remain

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
