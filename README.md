# VOXLINE

**VOXLINE** is a cross-platform vocal channel strip plugin for fast demo vocals and creator-friendly vocal processing.

The project is being rebuilt cleanly from the ground up.

## Product Goal

Build a beautiful, stable, easy-to-use vocal plugin that lets users quickly improve a vocal with:

```txt
Preset → POLISH → Small tone tweaks → Done
```

## Target Platforms

```txt
Windows: VST3
macOS: VST3 + AU
```

## Tech Stack

```txt
Framework: JUCE
Language: C++20
Build system: CMake
Platforms: Windows + macOS
Version control: Git
```

## MVP Controls

- GAIN
- AUTO GAIN
- POLISH
- BODY
- CLARITY
- AIR
- SMOOTH
- COMP
- DRIVE
- OUT
- BYPASS
- LISTEN

## Development Philosophy

Do not start by making the final Figma UI.

Build order:

```txt
1. Clean JUCE project
2. APVTS parameters
3. Functional debug UI
4. Minimal vocal DSP
5. Fixed Figma layout
6. Custom knobs
7. Themes
8. Meters / presets / polish
9. Cross-platform packaging
```

First make it build.  
Then make it move.  
Then make it sound.  
Then make it beautiful.

## Dependency

This project uses JUCE `8.0.12` via CMake `FetchContent`.

JUCE is fetched automatically during the first CMake configure step, so no local absolute path or manually installed JUCE SDK is required.

Requirements:

```txt
- Git
- CMake 3.22+
- Xcode 26+ on macOS or Visual Studio 2022 on Windows
```

## macOS Build

Generate an Xcode project:

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

Expected formats:

```txt
VOXLINE.vst3
VOXLINE.component
```

Suggested macOS install locations:

```txt
VST3: ~/Library/Audio/Plug-Ins/VST3/
AU:   ~/Library/Audio/Plug-Ins/Components/
```

## Windows Build

Generate a Visual Studio project:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected format:

```txt
VOXLINE.vst3
```

Suggested Windows VST3 install location:

```txt
C:\Program Files\Common Files\VST3\
```

## Testing

Minimum tests:

```txt
macOS:
- JUCE AudioPluginHost
- Reaper
- Logic Pro for AU if available
- auval when AU is ready

Windows:
- JUCE AudioPluginHost
- Reaper or another VST3 host
```

Phase 0 test checklist:

```txt
- CMake configure succeeds
- Current-platform plugin targets compile
- Plugin editor opens as a minimal shell
- No platform-specific path assumptions exist
```

General test checklist:

- Plugin opens
- Audio passes through
- Bypass works
- Controls move
- Parameters show in host automation
- State restores after closing and reopening
- No crash on open / close
- No loud pop when switching presets

## Signal Chain

Initial MVP chain:

```txt
Input
→ Input Gain
→ Body EQ
→ Clarity EQ
→ Air EQ
→ Smooth / De-esser placeholder
→ Compressor placeholder
→ Drive saturation
→ Auto Gain
→ Output Gain
→ Safety protection
→ Output
```

## Phase 3 Status

Phase 3 is now wired with safe placeholder DSP:

```txt
Smoothed Input Gain
→ Body low-shelf
→ Clarity presence peak
→ Air high-shelf
→ Smooth low-pass placeholder
→ One-knob compressor placeholder
→ Drive saturation
→ Auto gain trim
→ Smoothed Output Gain
→ Soft clip protection
```

Current verification:

```txt
- Release build passes
- Unit tests cover parameter/state/UI plus Phase 3 DSP behavior
- auval passes for the AU build
```

## Phase 4 Status

Phase 4 replaces the debug grid with the fixed `1100 x 760` VOXLINE layout from `UI.md`.

Current UI state:

```txt
- Fixed Top Bar / Input / Tone / Polish / Output / Bottom Bar sections
- Existing APVTS controls moved into fixed Figma coordinates
- POLISH remains the central largest real control
- Uses default JUCE controls for now; custom knob work starts in Phase 5
```

## Documentation

- `AGENT.md` — rules for the coding agent
- `TODO.md` — phase roadmap
- `UI.md` — Figma-based UI direction and fixed layout coordinates

## Git Rule

Every completed phase must be committed:

```bash
git status
git add .
git commit -m "<clear commit message>"
```
