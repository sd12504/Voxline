# VOXLINE 1.0

**VOXLINE** is a cross-platform vocal channel strip plugin for fast demo vocals and creator-friendly vocal processing.

Commercial release candidate of a dark-mode vocal channel strip for macOS and Windows.

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
- PRESENCE
- AIR
- DE-ESS
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

For an Intel-only binary (x86_64, macOS 11+):

```bash
cmake -B build-intel -G Xcode -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-intel --config Release
```

For a universal binary (Apple Silicon + Intel):

```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
```

Confirm the binary before distribution:

```bash
lipo -archs build/VOXLINE_artefacts/Release/VST3/VOXLINE.vst3/Contents/MacOS/VOXLINE
```

Expected formats:

```txt
VST3:  build/VOXLINE_artefacts/Release/VST3/VOXLINE.vst3
AU:    build/VOXLINE_artefacts/Release/AU/VOXLINE.component
```

### macOS Release Packaging

```bash
# Build first, then:
cd build && cpack -G DragNDrop    # creates VOXLINE-1.0.0-Darwin.dmg
```

Or manually:

```bash
mkdir -p pkg_root/VST3 pkg_root/Components
cp -R build/VOXLINE_artefacts/Release/VST3/VOXLINE.vst3 pkg_root/VST3/
cp -R build/VOXLINE_artefacts/Release/AU/VOXLINE.component pkg_root/Components/
pkgbuild --root pkg_root --install-location "/Library/Audio/Plug-Ins" \
  --identifier com.onetake.voxline --version 1.0.0 VOXLINE.pkg
hdiutil create -volname "VOXLINE" -srcfolder VOXLINE.pkg -ov -format UDZO VOXLINE_macOS.dmg
```

Suggested macOS install locations:

```txt
VST3: ~/Library/Audio/Plug-Ins/VST3/
AU:   ~/Library/Audio/Plug-Ins/Components/
```

## Windows Build

Generate a Visual Studio project:

```powershell
cmake -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected format:

```txt
VST3:  build\VOXLINE_artefacts\Release\VST3\VOXLINE.vst3
```

### Windows Release Packaging

```powershell
# Build first, then:
cd build; cpack -G ZIP    # creates VOXLINE-1.0.0-Windows.zip
```

Suggested Windows VST3 install location:

```txt
C:\Program Files\Common Files\VST3\
```

## Automated Release Builds

GitHub Actions builds and tests four release artifacts on every push and pull request:

- `VOXLINE-1.0.0-Windows-x64-VST3.zip`
- `VOXLINE-1.0.0-macOS-Apple-Silicon.dmg`
- `VOXLINE-1.0.0-macOS-Intel.dmg`
- `VOXLINE-1.0.0-macOS-Universal.dmg`

Pushing a tag such as `v1.0.0` also attaches those files to the corresponding GitHub Release.

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
→ Clean-mode rumble filter
→ Vocal EQ (HPF / Low / Mud / Presence / Air / LPF)
→ Split-band de-esser
→ Compressor (macro + advanced threshold/ratio/timing/mix)
→ Drive (macro + advanced tone/mix/character)
→ Space (ambience / slap / stereo wide)
→ Auto Gain compensation
→ Smoothed Output Gain
→ Soft clip protection
→ Bypass crossfade (5ms)
→ Output
```

POLISH acts as a multiplicative macro (0.35x-1.35x) scaling all tone/comp/drive intensity.

Listen mode outputs the difference signal (processed - dry) × 2 for auditioning.

## Current Status

V2 release candidate:

```txt
Phase 0: Clean JUCE project                    ✅
Phase 1: APVTS parameters (51 params)          ✅
Phase 2: Functional debug UI                   ✅
Phase 3: Minimal vocal DSP                     ✅
Phase 4: Simplified V2 layout (1080×720/900)   ✅
Phase 5: Custom knob system                    ✅
Phase 6: Premium dark-only interface           ✅
Phase 7: Meters, presets, visual polish        ✅
Phase 8: Cross-platform verification           ✅ (macOS + Windows CI)
```

Current verification:

```txt
- Release VST3 + AU build passes
- auval validates AU component
- All 51 APVTS parameters automatable
- Expandable Advanced EQ / Comp / De-ess / Drive / Space controls
- Real-time EQ spectrum display and coefficient-accurate response curve
- Editable EQ frequency, gain, Q and slope values
- Real input/output meters (peak + RMS)
- Gain reduction meter
- 9 DAW-visible factory presets plus user preset save/load
- Full-state A/B parameter snapshot comparison
- Dark-only commercial interface
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
