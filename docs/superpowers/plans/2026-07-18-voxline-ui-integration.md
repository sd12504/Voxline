# VOXLINE UI Integration Implementation Plan

> **For agentic workers:** Execute inline in this workspace. Do not dispatch subagents; the user explicitly requested a single implementer.

**Goal:** Connect the completed VOXLINE DSP, state, A/B, and user-preset systems to the agreed dark-only main and Advanced editor.

**Architecture:** Keep `VoxlineAudioProcessorEditor` as the existing JUCE editor boundary. Reuse its controls and JSON geometry where possible, replacing retired attachments and editor-owned state with the processor's registry, monitor, A/B, meters, and `PresetSessionController`.

**Tech Stack:** C++20, JUCE 8, APVTS attachments, existing Voxline UI controls and CTest unit tests.

## Global Constraints

- Only the dark UI is user-visible; no factory presets, FAV, global LISTEN, AUTO GAIN, Clean mode, or theme switch.
- Main controls use actual registered parameters; no display-only gain values or POLISH cross-control.
- Input and output meters render processor `MeterFrame` dBFS values directly.
- User Presets are user-only; A/B and monitor state are processor-owned and monitor state is transient.
- Preserve 1080x720 collapsed and 1080x940 Advanced dimensions.

---

### Task 1: Replace retired editor wiring and own UI session state

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`
- Modify: `Tests/EditorContractTests.cpp`

- [ ] Add a failing editor contract assertion that factory/retired controls are not exposed and active toolbar controls exist.
- [ ] Compile the test target; expect failure against the legacy editor source.
- [ ] Replace editor-local A/B snapshots and factory-preset file actions with `VoxlineAudioProcessor` A/B/monitor accessors and the existing `PresetSessionController`/`UserPresetLibrary`.
- [ ] Attach all main and Advanced controls to active registry IDs; remove visible retired Auto Gain, Clean, FAV, global Listen and theme affordances.
- [ ] Update callbacks so parameter edits mark the current user preset dirty, preset/A-B/page actions clear transient monitors, and unsaved actions use the controller's decision API.
- [ ] Run the focused editor contract test; expect PASS.
- [ ] Commit with `feat: connect editor controls to VOXLINE state`.

### Task 2: Render the agreed main controls, Advanced pages, and real meters

**Files:**
- Modify: `Source/PluginEditor.cpp`
- Modify: `Source/UI/layout.json`
- Modify: `Tests/EditorContractTests.cpp`

- [ ] Add failing source/UI contract coverage for dark-only toolbar, user preset empty state, ±12 dB EQ labels, five Space modes, and no old visible controls.
- [ ] Compile the test target; expect failure before the rendering update.
- [ ] Render Input Peak/RMS, Output L/R Peak/RMS/True Peak/clip state from `getInputMeterFrame()` and `getOutputMeterFrame()` without linear-to-dB conversion.
- [ ] Make main BODY/PRESENCE/AIR, DE-ESS, COMP, DRIVE, SPACE and POLISH reflect their registered parameters and live reduction/status values.
- [ ] Render Advanced EQ/Comp/De-Ess/Drive/Space controls using active IDs, with mode-dependent Space labels that fit on one line.
- [ ] Run focused UI tests and the editor test; expect PASS.
- [ ] Commit with `feat: render VOXLINE dark control surface`.

### Task 3: Verify editor behavior and plugin builds

**Files:**
- Modify if verification exposes a defect: `Source/PluginEditor.*`, `Tests/EditorContractTests.cpp`

- [ ] Run the complete CTest suite in Release.
- [ ] Build the macOS VST3 and AU targets.
- [ ] Inspect the built editor at both required dimensions and fix clipping, overlap, missing labels, or stale controls.
- [ ] Confirm only one inert host program remains and the user preset library contains no factory records.
- [ ] Commit verification fixes separately, if any.

## Coverage Review

- Toolbar, dark theme, dimensions, main processing controls, all five Advanced sections, Input/Output meters, user presets, A/B, and transient monitor clearing are covered by Tasks 1–2.
- Compilation, tests, visual geometry, and AU/VST3 builds are covered by Task 3.
