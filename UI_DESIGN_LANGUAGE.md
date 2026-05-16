# VOXLINE UI Design Language

This document defines the design language for all future VOXLINE UI work.

Scope:

- Applies to all editor-level visual updates
- Defines product feeling, hierarchy, layout, theme tokens, component rules, and JUCE implementation constraints
- Serves as the source of truth before any further UI refactors

This is a design system document, not a code task list.

---

## 1. Product Feeling

VOXLINE should look and feel like:

- premium vocal plugin
- boutique studio tool
- clean creator workflow
- soft, warm, modern
- fast to understand

VOXLINE should not look like:

- default JUCE
- toy-like plugin UI
- neon cyberpunk interface
- vintage hardware clone

Core visual sentence:

```txt
Soft boutique vocal tool with one clear hero control.
```

Product posture:

- Designed for vocal work, not generic channel-strip abstraction
- Immediate enough for creators, refined enough for mixing sessions
- Professional without becoming cold or technical-looking
- Distinct from commodity plugin skins

---

## 2. Visual Hierarchy

The UI hierarchy is fixed and intentional:

1. Brand: `VOXLINE`
2. Hero: `POLISH`
3. Tone Controls: `BODY / CLARITY / AIR / SMOOTH / COMP / DRIVE`
4. Workflow: `Preset + SPACE`
5. Utility: `A/B / Listen / Bypass / Theme / Auto Gain`
6. Monitoring: `Input dots / Output meters`

Hierarchy rules:

- `POLISH` is always the largest and most visually dominant control.
- Tone controls must read as a coherent secondary layer, never equal to `POLISH`.
- `SPACE` supports workflow and tone shaping, but must not visually compete with `POLISH`.
- Utility controls must be understated, legible, and fast to scan.
- Meters should feel professional and trustworthy, but cannot become the visual focal point.
- The logo should establish product identity quickly without stealing attention from the central workflow.

Attention order:

```txt
VOXLINE -> POLISH -> Tone knobs -> Preset / SPACE -> Utility -> Monitoring
```

---

## 3. Layout Rules

VOXLINE MVP layout is fixed-position, not responsive.

Editor and frame:

```txt
Editor fixed size: 1100 x 760
Main window: x=34 y=36 w=1032 h=668
```

Layout rules:

- Do not use FlexBox, CSS-like Grid, or proportional layout logic for MVP.
- Use fixed coordinates.
- Do not let panels auto-stretch.
- Do not change main panel coordinates unless explicitly requested.
- Keep cross-platform layout identical between macOS and Windows.
- Preserve spacing rhythm and panel relationships even when components are restyled.

Main panel coordinates:

```txt
Input Panel:  64,160,150,395
Tone Panel:   230,160,370,395
Polish Panel: 616,160,285,395
Output Panel: 916,160,130,395
Bottom Bar:   64,595,982,76
```

Structural guidance:

- Header remains visually light and horizontally calm.
- The main interaction band runs through the four vertical panels.
- The bottom bar acts as the workflow rail for preset, SPACE, and utility actions.
- Footer remains outside the main card emphasis and visually quiet.

---

## 4. Theme Tokens

All components must consume centralized theme tokens.

### Light Theme

```txt
Background:       #F3EFE8
Background 2:     #E6DED3
Main Window:      #FFF9F0
Panel:            #F7F0E7
Panel Border:     #DDD2C4

Text Primary:     #26222B
Text Secondary:   #746D7C
Text Muted:       #A198A8

Accent Rose:      #D86F96
Accent Peach:     #E99A5C
Accent Amber:     #D8A548
Accent Purple:    #8D70E8
Accent Lavender:  #B8A6F3

Meter High:       #E99A5C
Meter Mid:        #D86F96
Meter Low:        #8D70E8
```

### Dark Theme

```txt
Background:       #09080D
Background 2:     #14111D
Main Window:      #111018
Panel:            #181622
Panel Border:     #2A2635

Text Primary:     #F5F0EA
Text Secondary:   #9D96A8
Text Muted:       #6E6878

Accent Rose:      #E98BAA
Accent Peach:     #F2A766
Accent Amber:     #E6B45C
Accent Purple:    #A98CFF
Accent Lavender:  #C7B7FF

Meter High:       #F2A766
Meter Mid:        #E98BAA
Meter Low:        #A98CFF
```

Token rules:

- `Background` is the outer editor field.
- `Background 2` is reserved for subtle depth layers, gradients, and soft contrast zones.
- `Main Window` is the card surface.
- `Panel` is the default section surface.
- `Panel Border` is always thin and low-contrast.
- Accent usage must remain selective. Do not introduce unapproved colors.
- Blue is not part of the default VOXLINE accent system.

---

## 5. Panel Style

### Main Window

- corner radius: `28`
- soft shadow
- subtle border
- warm fill in light theme
- deep fill in dark theme

### Section Panels

- corner radius: `22`
- thin border
- subtle inner highlight
- soft shadow
- no hard outlines
- no flat gray JUCE panel treatment

### Bottom Bar

- corner radius: `24`
- same material language as section panels
- acts as workflow toolbar rather than a separate product layer

Panel rendering rules:

- Use soft depth, not sharp chrome.
- Borders must separate surfaces without looking boxed-in.
- Light theme should feel creamy and tactile.
- Dark theme should feel boutique and dimly lit, not aggressive.
- Surface contrast should be enough for legibility but never sterile.

---

## 6. Knob Style

All knobs must contain:

- circular body
- subtle inner gradient
- inactive arc
- active arc
- pointer line
- value text
- label text

Knob size hierarchy:

```txt
Hero POLISH: 205px
Medium knobs: 92px
Small output knob: 70px
```

Arc rules:

- Active arc uses an approved accent color.
- Inactive arc uses a low-contrast neutral.
- Arc thickness stays consistent across knob families.
- Active arc may use a subtle glow in dark theme.
- Do not use blue accents unless explicitly requested.
- Pointer line uses rounded caps.
- Knob drawing must remain fully inside component bounds.

Knob hierarchy rules:

- `POLISH` should feel premium, central, and calm.
- Tone knobs should feel consistent and readable as a set.
- Output knob should be visually lighter than the tonal controls.
- Labels must never collide with arcs or numeric readouts.

Typography for knobs:

- label: `11-12px`, semibold, muted
- value: `13-16px`, semibold, primary
- `POLISH` value: `28-32px`, bold

---

## 7. Button and Utility Style

The following controls are utility controls and must share one language:

- Preset dropdown
- SPACE control
- A/B
- Listen
- Bypass
- Theme
- Auto Gain

Utility style rules:

- pill or rounded shape
- no default JUCE look
- subtle border
- clear active state
- muted inactive state without looking disabled
- consistent internal padding
- consistent icon and text spacing
- icons must not have background boxes
- SVG assets must contain icon path or stroke only, with no rectangular background

State model:

```txt
Normal
Hover
Pressed
Active
Disabled
```

Active state:

- use Rose, Peach, or Purple family accents
- must be clear
- must not feel neon

Inactive state:

- must stay readable
- must be subdued
- must never appear broken or disabled unless actually disabled

Interaction posture:

- fast to target
- easy to understand at a glance
- visually quiet compared with `POLISH`

---

## 8. SPACE Control Style

`SPACE` is a compact bottom-bar workflow control. It is not a hero knob and must not live inside the `POLISH` panel.

Format reference:

```txt
SPACE  [ Tight ▼ ]  ━━━●────  24%
```

SPACE rules:

- placed in the center region of the Bottom Bar
- not rendered as a knob
- not blue
- uses Lavender, Purple, or Rose accents
- built as a compact pill-based control group
- amount slider uses custom draw calls
- type selector uses a small pill dropdown
- must not compete with `POLISH`

Design intent:

- quick ambience shaping
- compact and musical
- visually integrated with workflow actions
- supportive, not dominant

---

## 9. Meter Style

### Output Meters

- vertical rounded wells
- continuous rounded fill
- peak hold line
- soft gradient
- no segmented toy-meter styling unless explicitly art-directed
- `PEAK` and `RMS` text smaller but readable
- silence readout such as `-60.0` uses muted color
- `GR` meter is visually secondary to `OUT`

### Input Dots

- 7 small dots
- inactive dots remain visible but subtle
- active dots use Purple or Rose accents
- no text glyphs or encoding-style symbols

Meter rules:

- monitoring must feel dependable and studio-grade
- animation must be smooth and restrained
- meter color intensity must never overpower knob hierarchy
- values should read clearly in both themes

---

## 10. Typography

Recommended fonts:

- Inter
- SF Pro
- Manrope
- Avenir Next

Type hierarchy:

```txt
Logo: 32px / Bold / wide letter spacing
Subtitle: 12px / Medium
Panel title: 12-14px / Bold / uppercase / letter spaced
Knob label: 11-12px / SemiBold / uppercase
Knob value: 13-16px / SemiBold
POLISH value: 28-32px / Bold
Button text: 12-13px / SemiBold
Meter readout: 12px / tabular figures if possible
Footer: 10-11px / muted / letter spaced
```

Typography rules:

- do not use random font weights
- avoid default JUCE font appearance
- use tabular figures for numeric stability where possible
- labels must not touch arcs
- uppercase labels should feel controlled, not loud
- spacing should support a premium plugin feel rather than app-like density

---

## 11. Icon Rules

All icons must follow these rules:

- SVG path or stroke only
- no background rect
- no visible bounding boxes
- fixed icon size per control class
- icon and text spacing: `6-8px`
- icon color must come from theme text or accent tokens
- do not draw SVG to fill the entire button bounds

Reference sizes:

```txt
Bypass icon: 18-20px
Listen icon: 18-20px
Theme icon: 18-22px
```

Icon posture:

- clean
- minimal
- precise
- secondary to labels and state color

---

## 12. Interaction and Animation Rules

Interaction rules:

- knob changes should feel smooth
- meter updates should use smoothing
- buttons require hover and pressed feedback
- theme switching may be instant or lightly faded
- avoid distracting animation
- avoid heavy GPU effects
- no motion that competes with audio work

Motion language:

- subtle over dramatic
- responsive over decorative
- functional over playful

Animation constraints:

- transitions should support readability
- no bouncing, elastic, or game-like movement
- glow effects must remain restrained

---

## 13. Footer

Footer format:

```txt
VOXLINE 1.0.0  |  SADTONY
```

Footer rules:

- centered at the bottom
- very muted
- letter spaced
- must not compete with the Bottom Bar
- compatible with both Light and Dark themes

---

## 14. Do and Don't

### Do

- Use warm cream and deep purple-black as the base atmosphere.
- Use Rose, Peach, and Lavender accents selectively.
- Keep `POLISH` as the hero.
- Keep workflow simple and legible.
- Use custom JUCE drawing.
- Keep controls readable and premium.

### Don't

- Do not use default JUCE components visually.
- Do not use blue accent for `SPACE`.
- Do not use artist names in presets.
- Do not place `SPACE` inside the `POLISH` panel.
- Do not make utility buttons louder than `POLISH`.
- Do not add random new colors.
- Do not change the fixed layout without approval.
- Do not use whole-UI image composites as the implementation.
- Do not make the plugin look like an old hardware clone.

---

## 15. Implementation Notes

Notes for the JUCE implementation agent:

- `Theme.h` and `Theme.cpp` should contain theme tokens.
- `Layout.h` should contain fixed coordinates.
- Components should read from centralized theme tokens.
- Do not place magic colors inside component paint methods.
- Do not duplicate layout numbers inside paint methods.
- All custom components must respect their bounds.
- No component should draw outside its bounds.
- All APVTS attachments must be member variables.

Implementation discipline:

- Design tokens are authoritative.
- Layout constants are authoritative.
- Styling decisions should be reusable, not one-off.
- Custom drawing is required for premium presentation.
- The UI should be implemented as composable JUCE components, not as a pasted mockup.

---

## 16. Governance

Any future UI modification should be evaluated against this document before code changes are made.

Approval is required before changing:

- main panel coordinates
- control hierarchy
- theme accent family
- `POLISH` prominence
- `SPACE` placement model

When in doubt, preserve:

```txt
Soft boutique vocal tool with one clear hero control.
```
