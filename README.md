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

```txt
Input
→ Smoothed Input Gain
→ Body EQ (bell 200Hz, -4/+5dB)
→ Clarity EQ (peak 3.5kHz, -3/+6dB)
→ Air EQ (high shelf 7kHz, -3/+7dB)
→ Smooth (high shelf cut 6kHz, 0/-6dB)
→ Compressor (one-knob, -12/-32dB thr, 1.2:1-5:1)
→ Drive (tanh saturation, 0-12dB)
→ Auto Gain compensation
→ Smoothed Output Gain
→ Soft clip protection
→ Bypass crossfade (5ms)
→ Output
```

POLISH acts as a multiplicative macro (0.35x-1.35x) scaling all tone/comp/drive intensity.

Listen mode outputs the difference signal (processed - dry) × 2 for auditioning.

## Current Status

All 8 phases complete:

```txt
Phase 0: Clean JUCE project                    ✅
Phase 1: APVTS parameters                      ✅
Phase 2: Functional debug UI                   ✅
Phase 3: Minimal vocal DSP                     ✅
Phase 4: Fixed Figma layout (1100×760)         ✅
Phase 5: Custom knob system                    ✅
Phase 6: Light/Dark theme system               ✅
Phase 7: Meters, presets, visual polish        ✅
Phase 8: Cross-platform verification           ✅ (macOS)
```

Current verification:

```txt
- Release VST3 + AU build passes (3.7MB)
- auval validates AU component
- All 12 APVTS parameters automatable
- Real input/output meters (peak + RMS)
- Gain reduction meter
- 9 artist presets (Clean + 8 signatures)
- A/B parameter snapshot comparison
- Dark/Light theme toggle
- Bypass with smooth crossfade
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
