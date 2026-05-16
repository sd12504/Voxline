# VOXLINE — Complete Vocal Processing Plugin

## Product Identity

VOXLINE is a complete vocal processing plugin for fast, polished vocal chains.
中文：VOXLINE 是一款完整人聲處理插件，讓創作者快速完成可用、乾淨、有風格的人聲鏈。

UI header remains: **Fast Vocal Channel** (short, fits UI)

## Core Workflow

1. Choose a **Preset** — built by engineers, immediately usable
2. Dial **POLISH** — global macro, controls overall processing intensity
3. Fine-tune **TONE** / **SPACE** — minimal tweaks if needed
4. Done

Target user: creators who want a usable vocal sound without opening 5 plugins.

## Module Architecture

| Module | Controls | DSP Foundation | Status |
|--------|----------|----------------|--------|
| **INPUT** | inputGain, autoGain, cleanMode | Gain staging, noise gate? | ⚠️ cleanMode unimplemented |
| **TONE** | body, clarity, air, smooth | IIR filters (peak/shelf) | ✅ Basic, can strengthen |
| **DYNAMICS** | comp, GR meter | Single compressor model | ⚠️ One model only |
| **COLOR** | drive, saturation/warmth | Soft clip + ? | ⚠️ sat/warmth unimplemented |
| **POLISH** | polish (global macro) | Scales all processing intensity | ✅ |
| **SPACE** | spaceAmount, spaceType (3 modes) | Filtered delay network | ⚠️ Needs refinement |
| **OUTPUT** | outputGain, peak/rms, bypass, listen, A/B | Gain, metering, bypass | ✅ |


## Design Principles

- **One-page UI** — no tabs, no advanced panels, no complexity creep
- **Creator-friendly** — you don't need to understand mixing to get a good sound
- **Preset-first** — the quickest path to a good sound is choosing the right preset
- **POLISH is the star** — one knob that makes everything work together
- **SPACE is NOT a reverb** — vocal ambience, slap, width. No hall, no room, no plate
- **TONE stays simple** — four named knobs, not a parametric EQ

## What VOXLINE Is Not

- Not a channel strip (it's more than that — it's a complete chain)
- Not a parametric EQ (TONE is intentionally simplified)
- Not a reverb plugin (SPACE is filtered, conservative, voice-first)
- Not an engineer's tool (no sidechain, no external routing, no advanced metering)

## Visual Identity

- Header: "VOXLINE" + "Fast Vocal Channel"
- Footer: "VOXLINE 1.0.0 | SADTONY"
- Theme: Light/Dark toggle
- Layout: Single card, ~1100×760
