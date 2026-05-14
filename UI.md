# UI.md — VOXLINE Cross-Platform Figma-Based UI Specification

## 0. Reference

This document is based on the current VOXLINE Figma design direction.

```txt
Figma Prototype:
https://www.figma.com/proto/Q2RbWzNtcBDVkgzOFMtxsY/ONETAKE-VOICE---Premium-Vocal-Channel-Strip-UI?node-id=9-2&t=rHaBJEH3EEdHQlwd-1&scaling=min-zoom&content-scaling=fixed&page-id=0%3A1

Figma Design:
https://www.figma.com/design/Q2RbWzNtcBDVkgzOFMtxsY/ONETAKE-VOICE---Premium-Vocal-Channel-Strip-UI?node-id=9-2
```

Main Figma frames to reference:

```txt
VOXLINE / Image Reference Improved / Light
VOXLINE / Image Reference Improved / Dark
VOXLINE Tokens + JUCE Handoff
```

Target implementation:

```txt
JUCE C++ audio plugin UI
Windows VST3
macOS VST3 / AU
```

Important:

```txt
Do not implement this UI as React / HTML / CSS.
Do not paste the whole Figma design as one image.
Recreate the UI with JUCE custom components and draw calls.
Use exported images only for optional icons, logo marks, or subtle texture.
```

---

# 1. Product Identity

## Product Name

```txt
VOXLINE
```

## Subtitle

```txt
Fast Vocal Channel
```

## Product Type

```txt
Modern vocal channel plugin
```

## Target Users

```txt
Singers
Rappers
Creators
Demo producers
Bedroom producers
Vocal editors
Music makers who want a fast vocal chain
```

## Product Promise

```txt
Put VOXLINE on a vocal track.
Choose a preset.
Turn POLISH.
Fine-tune Body / Clarity / Air / Smooth / Comp / Drive only if needed.
Done.
```

The interface should communicate:

```txt
Fast
Premium
Musical
Easy
Creator-friendly
Not overly technical
```

---

# 2. Visual Direction

## Overall Mood

VOXLINE should feel:

```txt
Premium
Modern
Beautiful
Creator-friendly
Minimal
Soft
Musical
Fast to understand
Boutique
Warm
Clean
High-end
```

VOXLINE should not feel:

```txt
Default JUCE
Old hardware clone
Cheap metal imitation
Overcrowded
Engineering-only
Hard to understand
Toy-like
Too flat
Too neon
```

## Core Visual Concept

```txt
A beautiful vocal plugin with one large POLISH macro knob and a small number of meaningful controls around it.
```

The design should use:

```txt
Rounded panels
Soft shadows
Thin borders
Pastel accent colors
Elegant rotary knobs
Readable labels
Clean preset bar
Output meters on the right
```

---

# 3. Themes

VOXLINE has two official themes:

```txt
Light Theme
Dark Theme
```

Both themes must share the same layout and component structure.

The difference should come from:

```txt
Theme colors
Panel fills
Text colors
Shadow intensity
Glow intensity
Meter contrast
```

## Theme Priority

```txt
Light theme: clean cream product look
Dark theme: boutique night studio product look
```

Dark theme is not optional.  
It must be implemented as part of the main UI system.

---

# 4. Canvas / Plugin Size

## Figma Design Size

```txt
1100 x 760 px
```

## JUCE Editor Size

```cpp
setSize (1100, 760);
```

## Suggested Size Limits

```txt
Minimum: 950 x 660 px
Default: 1100 x 760 px
Maximum: 1400 x 960 px
```

For MVP, use a fixed-size editor on both Windows and macOS.

Resizable UI can be added later only after the fixed Figma layout is visually correct.
Do not let panels stretch vertically or horizontally during the MVP UI phase.

The same layout coordinates must be used on Windows and macOS so the plugin does not drift between platforms.

---

# 5. Main Layout

## High-Level Structure

```txt
┌──────────────────────────────────────────────────────────────┐
│ Top Bar                                                      │
│ VOXLINE / Fast Vocal Channel       Preset / Bypass / Setting │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│ INPUT     TONE CONTROLS              POLISH        OUTPUT    │
│                                                              │
│ Gain      Body   Clarity   Air         Hero         OUT Meter │
│ Auto      Smooth Comp      Drive       Knob         GR Meter  │
│ Clean                                65%            Out Knob  │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ Bottom Preset Bar                                            │
│ Clean       Warm       Bright       Rap        A/B   Listen   │
└──────────────────────────────────────────────────────────────┘
```

## Main Sections

```txt
1. Top Bar
2. INPUT Panel
3. TONE CONTROLS Panel
4. POLISH Hero Panel
5. OUTPUT Panel
6. Bottom Preset Bar
```

## Visual Priority

The user should see the interface in this order:

```txt
1. VOXLINE brand
2. Preset: Warm
3. POLISH hero knob
4. Tone controls
5. Output meters
6. Input / utility controls
7. A/B / Listen / Settings
```


## 5.1 Fixed Coordinate Layout — MVP / Figma Match

For the first JUCE implementation, use fixed absolute coordinates based on the Figma frame.
This is intentional.
Do not use FlexBox, Grid, equal distribution, or proportional stretching until the Figma match is approved.

```cpp
setSize (1100, 760);
```

### Layout Principle

```txt
The UI should be positioned like the Figma reference, not stretched to fill the editor.
Panels must keep a card-like proportion.
Do not allow the main panels to become tall vertical columns.
POLISH must remain the visual center and largest control.
Bottom buttons must sit inside the bottom preset bar, not directly on the editor edge.
```

---

### Global Areas

```txt
Editor:
x = 0
y = 0
w = 1100
h = 760

Main Window / Card:
x = 34
y = 36
w = 1032
h = 668
radius = 28

Header Area:
x = 64
y = 58
w = 982
h = 58

Main Panel Row:
x = 64
y = 160
w = 982
h = 395

Bottom Preset Bar:
x = 64
y = 595
w = 982
h = 76
radius = 24
```

---

### Header Coordinates

```txt
Logo Text VOXLINE:
x = 64
y = 62
w = 230
h = 34

Subtitle Fast Vocal Channel:
x = 66
y = 96
w = 180
h = 18

Preset Selector:
x = 705
y = 62
w = 180
h = 42

Bypass Button:
x = 905
y = 62
w = 86
h = 42

Settings Button:
x = 1004
y = 62
w = 42
h = 42
```

---

### Main Panel Coordinates

```txt
Input Panel:
x = 64
y = 160
w = 150
h = 395
radius = 22

Tone Controls Panel:
x = 230
y = 160
w = 370
h = 395
radius = 22

Polish Panel:
x = 616
y = 160
w = 285
h = 395
radius = 22

Output Panel:
x = 916
y = 160
w = 130
h = 395
radius = 22
```

These four panel rectangles are the most important positioning rules.
If the JUCE build looks too tall, stretched, or empty, check these values first.

---

### INPUT Panel Internal Coordinates

```txt
INPUT Title:
x = 64
y = 186
w = 150
h = 24

Input Gain Knob:
x = 93
y = 260
w = 92
h = 92

Input Gain Value:
x = 64
y = 366
w = 150
h = 22

Input Gain Label:
x = 64
y = 388
w = 150
h = 18

Auto Gain Toggle:
x = 82
y = 425
w = 46
h = 24

Auto Gain Label:
x = 136
y = 426
w = 78
h = 22

Clean Mode Button:
x = 88
y = 476
w = 104
h = 38

Input LED Dots:
x = 96
y = 532
w = 86
h = 16
```

---

### TONE CONTROLS Panel Internal Coordinates

```txt
TONE CONTROLS Title:
x = 230
y = 188
w = 370
h = 26

Optional Title Divider Left:
x = 258
y = 201
w = 90
h = 1

Optional Title Divider Right:
x = 482
y = 201
w = 90
h = 1

Body Knob:
x = 265
y = 246
w = 92
h = 92

Clarity Knob:
x = 385
y = 246
w = 92
h = 92

Air Knob:
x = 505
y = 246
w = 92
h = 92

Smooth Knob:
x = 265
y = 386
w = 92
h = 92

Comp Knob:
x = 385
y = 386
w = 92
h = 92

Drive Knob:
x = 505
y = 386
w = 92
h = 92
```

Knob value and label text should be drawn inside each knob component or directly below the arc.
Keep the same text spacing across all six tone knobs.

---

### POLISH Panel Internal Coordinates

```txt
POLISH Title:
x = 616
y = 188
w = 285
h = 26

Hero Polish Knob:
x = 656
y = 220
w = 205
h = 205

Polish Value:
x = 616
y = 440
w = 285
h = 42

Polish Label:
x = 616
y = 482
w = 285
h = 24
```

POLISH must not collapse into only a value label.
The hero knob must be visible, centered, and visually dominant.

---

### OUTPUT Panel Internal Coordinates

```txt
OUTPUT Title:
x = 916
y = 186
w = 130
h = 24

PEAK / RMS Text:
x = 936
y = 216
w = 88
h = 44

OUT Meter:
x = 958
y = 270
w = 18
h = 165

GR Meter:
x = 990
y = 270
w = 18
h = 165

OUT / GR Meter Labels:
x = 940
y = 446
w = 88
h = 22

Output Gain Knob:
x = 946
y = 476
w = 70
h = 70

Output Value:
x = 916
y = 552
w = 130
h = 20

Output Label:
x = 916
y = 572
w = 130
h = 18
```

Avoid placing the output readouts and OUT knob at the very bottom of the panel.
The meters should live in the upper/middle area of the output panel.

---

### Bottom Preset Bar Coordinates

```txt
Bottom Preset Bar:
x = 64
y = 595
w = 982
h = 76

Clean Button:
x = 94
y = 616
w = 104
h = 36

Warm Button:
x = 230
y = 616
w = 104
h = 36

Bright Button:
x = 366
y = 616
w = 104
h = 36

Rap Button:
x = 502
y = 616
w = 104
h = 36

A/B Button:
x = 862
y = 616
w = 82
h = 36

Listen Button:
x = 958
y = 616
w = 82
h = 36
```

Do not add long helper text inside the bottom bar for the MVP.
The Figma direction is cleaner when the bar only contains quick preset and utility buttons.

---

### Suggested JUCE Layout Constants

Create a dedicated layout file so the editor does not contain magic numbers everywhere.

```cpp
#pragma once
#include <JuceHeader.h>

namespace VoxlineLayout
{
    static constexpr int editorW = 1100;
    static constexpr int editorH = 760;

    static constexpr int mainX = 34;
    static constexpr int mainY = 36;
    static constexpr int mainW = 1032;
    static constexpr int mainH = 668;

    static constexpr int panelY = 160;
    static constexpr int panelH = 395;

    static constexpr juce::Rectangle<int> inputPanel  { 64,  panelY, 150, panelH };
    static constexpr juce::Rectangle<int> tonePanel   { 230, panelY, 370, panelH };
    static constexpr juce::Rectangle<int> polishPanel { 616, panelY, 285, panelH };
    static constexpr juce::Rectangle<int> outputPanel { 916, panelY, 130, panelH };

    static constexpr juce::Rectangle<int> bottomBar { 64, 595, 982, 76 };

    static constexpr juce::Rectangle<int> logo       { 64, 62, 230, 34 };
    static constexpr juce::Rectangle<int> subtitle   { 66, 96, 180, 18 };
    static constexpr juce::Rectangle<int> preset     { 705, 62, 180, 42 };
    static constexpr juce::Rectangle<int> bypass     { 905, 62, 86, 42 };
    static constexpr juce::Rectangle<int> settings   { 1004, 62, 42, 42 };

    static constexpr juce::Rectangle<int> inputKnob  { 93, 260, 92, 92 };
    static constexpr juce::Rectangle<int> autoToggle { 82, 425, 46, 24 };
    static constexpr juce::Rectangle<int> inputClean { 88, 476, 104, 38 };
    static constexpr juce::Rectangle<int> inputDots  { 96, 532, 86, 16 };

    static constexpr juce::Rectangle<int> bodyKnob    { 265, 246, 92, 92 };
    static constexpr juce::Rectangle<int> clarityKnob { 385, 246, 92, 92 };
    static constexpr juce::Rectangle<int> airKnob     { 505, 246, 92, 92 };
    static constexpr juce::Rectangle<int> smoothKnob  { 265, 386, 92, 92 };
    static constexpr juce::Rectangle<int> compKnob    { 385, 386, 92, 92 };
    static constexpr juce::Rectangle<int> driveKnob   { 505, 386, 92, 92 };

    static constexpr juce::Rectangle<int> polishKnob  { 656, 220, 205, 205 };

    static constexpr juce::Rectangle<int> outMeter    { 958, 270, 18, 165 };
    static constexpr juce::Rectangle<int> grMeter     { 990, 270, 18, 165 };
    static constexpr juce::Rectangle<int> outputKnob  { 946, 476, 70, 70 };

    static constexpr juce::Rectangle<int> cleanButton  { 94, 616, 104, 36 };
    static constexpr juce::Rectangle<int> warmButton   { 230, 616, 104, 36 };
    static constexpr juce::Rectangle<int> brightButton { 366, 616, 104, 36 };
    static constexpr juce::Rectangle<int> rapButton    { 502, 616, 104, 36 };
    static constexpr juce::Rectangle<int> abButton     { 862, 616, 82, 36 };
    static constexpr juce::Rectangle<int> listenButton { 958, 616, 82, 36 };
}
```

### Suggested `resized()` Rule

```cpp
void PluginEditor::resized()
{
    logoLabel.setBounds (VoxlineLayout::logo);
    subtitleLabel.setBounds (VoxlineLayout::subtitle);
    presetSelector.setBounds (VoxlineLayout::preset);
    bypassButton.setBounds (VoxlineLayout::bypass);
    settingsButton.setBounds (VoxlineLayout::settings);

    inputPanel.setBounds (VoxlineLayout::inputPanel);
    tonePanel.setBounds (VoxlineLayout::tonePanel);
    polishPanel.setBounds (VoxlineLayout::polishPanel);
    outputPanel.setBounds (VoxlineLayout::outputPanel);
    presetBar.setBounds (VoxlineLayout::bottomBar);

    inputGainKnob.setBounds (VoxlineLayout::inputKnob);
    autoGainToggle.setBounds (VoxlineLayout::autoToggle);
    inputCleanButton.setBounds (VoxlineLayout::inputClean);

    bodyKnob.setBounds (VoxlineLayout::bodyKnob);
    clarityKnob.setBounds (VoxlineLayout::clarityKnob);
    airKnob.setBounds (VoxlineLayout::airKnob);
    smoothKnob.setBounds (VoxlineLayout::smoothKnob);
    compKnob.setBounds (VoxlineLayout::compKnob);
    driveKnob.setBounds (VoxlineLayout::driveKnob);

    polishKnob.setBounds (VoxlineLayout::polishKnob);

    outMeter.setBounds (VoxlineLayout::outMeter);
    grMeter.setBounds (VoxlineLayout::grMeter);
    outputGainKnob.setBounds (VoxlineLayout::outputKnob);

    cleanPresetButton.setBounds (VoxlineLayout::cleanButton);
    warmPresetButton.setBounds (VoxlineLayout::warmButton);
    brightPresetButton.setBounds (VoxlineLayout::brightButton);
    rapPresetButton.setBounds (VoxlineLayout::rapButton);
    abButton.setBounds (VoxlineLayout::abButton);
    listenButton.setBounds (VoxlineLayout::listenButton);
}
```


---

# 6. Top Bar

## Left Side

```txt
VOXLINE
Fast Vocal Channel
```

## Right Side

```txt
Preset selector: Preset: Warm ▼
Bypass button
Settings button
```

## Design Rules

```txt
VOXLINE should use wide letter spacing.
Subtitle should be small and understated.
Preset selector should be a rounded pill.
Bypass should be visible but not dominant.
Settings should be a small rounded icon button.
```

## Typography

```txt
Logo: 32 px / Bold / letter spacing 3.0–3.5 px
Subtitle: 12 px / Medium
Preset: 14 px / Semi Bold
Buttons: 12 px / Semi Bold
```

---

# 7. INPUT Panel

## Purpose

The INPUT panel handles basic vocal input preparation.

## Required Controls

```txt
GAIN knob
AUTO GAIN toggle
Clean pill button
Small input LED indicator
```

## GAIN

```txt
Label: GAIN
Example value: +2.5
Type: Rotary knob
Size: Medium
Accent: Lavender / Purple
```

## AUTO GAIN

```txt
Label: AUTO GAIN
Type: Toggle switch
Default state: On
Accent: Rose / Peach gradient
```

## Clean Button

```txt
Label: Clean
Type: Pill button
State: Active / highlighted
Accent: Rose / Peach gradient
```

## Input LED Indicator

```txt
Small decorative input level indicator.
Can be 6–8 tiny LED dots.
First few dots active, remaining dots muted.
Do not build a full input meter for MVP.
```

---

# 8. TONE CONTROLS Panel

## Purpose

The TONE CONTROLS panel provides simple, creator-friendly vocal shaping.

## Panel Title

```txt
TONE CONTROLS
```

## Layout

```txt
Top row:
BODY / CLARITY / AIR

Bottom row:
SMOOTH / COMP / DRIVE
```

## Controls

```txt
BODY
CLARITY
AIR
SMOOTH
COMP
DRIVE
```

All controls should be rotary knobs.

---

## 8.1 BODY

```txt
Purpose: Adds or reduces vocal thickness.
Label: BODY
Range: 0% – 100%
Example value: 54%
Accent: Peach / Amber
Meaning: Lower = thinner, Higher = fuller
```

## 8.2 CLARITY

```txt
Purpose: Controls vocal forwardness and intelligibility.
Label: CLARITY
Range: 0% – 100%
Example value: 61%
Accent: Rose / Peach
```

Use `CLARITY` instead of `Presence` because it is easier for non-engineers.

## 8.3 AIR

```txt
Purpose: Controls brightness and high-frequency vocal shine.
Label: AIR
Range: 0% – 100%
Example value: 48%
Accent: Lavender / Purple
```

## 8.4 SMOOTH

```txt
Purpose: De-esser / harshness reduction.
Label: SMOOTH
Range: 0% – 100%
Example value: 32%
Accent: Rose
```

Do not show the word `De-esser` prominently on the main UI.

## 8.5 COMP

```txt
Purpose: One-knob compression amount.
Label: COMP
Range: 0% – 100%
Example value: 42%
Accent: Purple
```

## 8.6 DRIVE

```txt
Purpose: Saturation / warmth / harmonic color.
Label: DRIVE
Range: 0% – 100%
Example value: 18%
Accent: Amber / Peach
```

---

# 9. POLISH Hero Panel

## Purpose

POLISH is the central macro control and the most important control in VOXLINE.

## UI

```txt
Panel title: POLISH
Control type: Large rotary knob
Range: 0% – 100%
Example value: 65%
Accent: Rose / Pink
```

## Visual Rules

```txt
POLISH must be the largest knob on the screen.
POLISH must feel visually separated from the smaller tone controls.
POLISH should have the strongest soft glow.
POLISH should use a rose-pink active arc.
POLISH should show a large value below the knob.
```

## Recommended Size

```txt
Large POLISH knob: 180–220 px
Current Figma direction: around 210 px
```

## Behavior

```txt
Dragging POLISH should update the macro value.
Knob arc should animate smoothly.
Value text should update in real time.
```

---

# 10. OUTPUT Panel

## Purpose

The OUTPUT panel shows the final signal level and gain reduction.

## Required Elements

```txt
OUT meter
GR meter
OUT knob
PEAK readout
RMS readout
```

## OUT Meter

```txt
Label: OUT
Type: Vertical meter
Role: Output level
Visual priority: Higher than GR
```

## GR Meter

```txt
Label: GR
Type: Vertical meter
Role: Gain reduction
Visual priority: Secondary
```

## OUT Knob

```txt
Label: OUT
Example value: -1.0
Type: Rotary knob
Size: Small
Accent: Lavender / Neutral Purple
```

## PEAK / RMS Readouts

```txt
PEAK -3.4
RMS -14.8
```

These can be static placeholders during UI phase, then connected to real metering later.

## Meter Style

Meters should be:

```txt
Rounded
Soft
Elegant
Gradient-filled
Smoothly animated
Not aggressive
Not too bright
```

---

# 11. Bottom Preset Bar

## Purpose

The bottom bar provides fast preset-style entry points and utility buttons.

## Required Buttons

```txt
Clean
Warm
Bright
Rap
A/B
Listen
```

## Active State

```txt
Warm is the active preset in the Figma reference.
```

## Layout

```txt
Clean / Warm / Bright / Rap on the left and center.
A/B / Listen on the right.
```

## Design Rules

```txt
Use rounded pill buttons.
Active preset should use Rose → Peach gradient.
Inactive presets should be low contrast but readable.
Do not place long helper text in the bottom bar.
Keep the bar clean and premium.
```

---

# 12. Light Theme Tokens

Use this for:

```txt
VOXLINE / Image Reference Improved / Light
```

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

Warning:          #D95050
Success:          #4FAF7A
```

## Light Theme Notes

```txt
Avoid pure white.
Use cream and ivory.
Keep text contrast strong.
Use warm-gray borders.
Use soft shadows.
Accents should be pastel but readable.
```

---

# 13. Dark Theme Tokens

Use this for:

```txt
VOXLINE / Image Reference Improved / Dark
VOXLINE Tokens + JUCE Handoff
```

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

Warning:          #FF6B6B
Success:          #84E6B0
```

## Dark Theme Notes

```txt
Dark theme should feel boutique / night studio / premium.
Avoid pure black everywhere.
Use deep purple-black panels.
Use soft glow only around active controls.
Do not overuse neon.
Keep text readable.
Use slightly stronger glow than light theme.
Warm active buttons can keep the rose → peach gradient.
```

---

# 14. Shared Component Design

## Panels

```txt
Main window radius: 28 px
Inner panels radius: 22 px
Bottom bar radius: 24 px
Thin border
Soft drop shadow
Subtle fill difference between main window and panel
```

## Knobs

Each knob should include:

```txt
Circular body
Soft shadow
Subtle highlight
Inactive arc
Active colored arc
Small pointer line
Value text
Uppercase label
```

Recommended sizes:

```txt
Small knob: 72 px
Medium knob: 78–92 px
Hero POLISH knob: 180–220 px
```

## Meters

```txt
Vertical rounded bars
Soft gradient fill
Thin border
Peak hold line
Muted inactive well
Small label underneath
```

## Buttons

```txt
Rounded pill shape
Active state: Rose → Peach gradient
Inactive state: panel fill + thin border
Utility buttons should stay visually secondary
```

---

# 15. Typography

Use a clean modern sans-serif.

Recommended:

```txt
Inter
SF Pro
Avenir Next
Manrope
Satoshi
```

## Text Hierarchy

```txt
Logo:             32 px / Bold / wide tracking
Subtitle:         12 px / Medium
Panel titles:     12–14 px / Bold / uppercase / letter spaced
Knob labels:      11–12 px / Semi Bold / uppercase
Knob values:      12–13 px / Semi Bold
POLISH value:     28–32 px / Bold
Preset buttons:   12 px / Semi Bold
Readouts:         12 px / Semi Bold
```

---

# 16. JUCE Implementation Strategy

## Required UI Classes

```txt
PluginEditor
ThemeManager
CustomKnob
HeroKnob
LevelMeter
GainReductionMeter
SectionPanel
PresetSelector
PillButton
ToggleSwitch
TopBar
OutputReadout
```

## Suggested File Structure

```txt
Source/
  PluginProcessor.h
  PluginProcessor.cpp
  PluginEditor.h
  PluginEditor.cpp

  UI/
    Theme.h
    Theme.cpp
    Layout.h
    CustomKnob.h
    CustomKnob.cpp
    HeroKnob.h
    HeroKnob.cpp
    LevelMeter.h
    LevelMeter.cpp
    SectionPanel.h
    SectionPanel.cpp
    PillButton.h
    PillButton.cpp
    ToggleSwitch.h
    ToggleSwitch.cpp
    TopBar.h
    TopBar.cpp
```

## Use JUCE Drawing For

```txt
Panels
Background gradients
Rounded rectangles
Borders
Knob arcs
Knob bodies
Meter wells
Meter fills
Buttons
Text
Soft glow
Shadows
LED dots
```

## Use SVG / PNG Only For

```txt
Optional logo mark
Small icons
Optional subtle texture
```

Do not use images for:

```txt
Whole UI
Text
Meters
Knob values
Dynamic controls
```

---

# 17. Parameters / Control Mapping

## Suggested Parameters

```txt
inputGain
autoGain
polish
body
clarity
air
smooth
comp
drive
outputGain
bypass
listen
```

## Suggested Parameter Ranges

```txt
inputGain:  -24 dB to +24 dB
outputGain: -24 dB to +24 dB

polish:     0% to 100%
body:       0% to 100%
clarity:    0% to 100%
air:        0% to 100%
smooth:     0% to 100%
comp:       0% to 100%
drive:      0% to 100%

autoGain:   on/off
bypass:     on/off
listen:     on/off
```

---

# 18. MVP Scope

## Must Include

```txt
VOXLINE logo text
Fast Vocal Channel subtitle
Preset selector
Bypass button
Settings button

INPUT panel
GAIN knob
AUTO GAIN toggle
Clean button
Input LED dots

TONE CONTROLS panel
BODY knob
CLARITY knob
AIR knob
SMOOTH knob
COMP knob
DRIVE knob

POLISH hero knob

OUTPUT panel
OUT meter
GR meter
OUT knob
PEAK / RMS readouts

Bottom preset bar
Clean / Warm / Bright / Rap buttons
A/B button
Listen button

Light theme
Dark theme
ThemeManager
```

## Can Postpone

```txt
Advanced page
Theme browser UI
Resizable UI
Animated background
Detailed input meter
De-esser listen mode
Preset management system
Real RMS calibration
Full accessibility pass
```

---

# 19. Development Rules for Agent

The coding agent must follow these rules:

```txt
1. Do not replace the UI with a PNG screenshot.
2. Do not implement this as React / HTML / CSS.
3. Use JUCE C++ custom components.
4. Recreate the Figma design with draw calls and real controls.
5. Keep POLISH as the visual hero.
6. Keep labels creator-friendly.
7. Use UI.md colors as theme tokens.
8. Implement both Light and Dark themes in ThemeManager.
9. Commit after every completed stage.
10. Do not change product direction without asking.
```

---

# 20. Suggested Implementation Stages

## Stage 0 — Functional Debug UI First

```txt
Before final Figma UI, create default JUCE sliders/buttons for all APVTS parameters.
Confirm every control moves.
Confirm attachments are PluginEditor members.
Confirm parameter values update and persist.
Commit before moving to the Figma layout.
```

## Stage 1 — UI Foundation

```txt
Create ThemeManager
Create LightTheme and DarkTheme token structs
Create layout constants in UI/Layout.h
Set PluginEditor size to 1100 x 760
Use the fixed coordinate layout from section 5.1
Draw background and main window
Add TopBar text placeholders
Keep controls interactive
Commit
```

## Stage 2 — Panels

```txt
Create SectionPanel component
Add InputPanel at x=64, y=160, w=150, h=395
Add ToneControlsPanel at x=230, y=160, w=370, h=395
Add PolishPanel at x=616, y=160, w=285, h=395
Add OutputPanel at x=916, y=160, w=130, h=395
Add PresetBar at x=64, y=595, w=982, h=76
Commit
```

## Stage 3 — Knobs

```txt
Create CustomKnob component that inherits juce::Slider or otherwise remains a real interactive control
Create HeroKnob variation for POLISH
Do not create passive fake knob Components
Add all knob labels and values
Connect knobs to APVTS parameters
Confirm drag interaction, value text, active arc, and automation still work
Commit
```

## Stage 4 — Buttons and Toggle

```txt
Create PillButton component
Create ToggleSwitch component
Add preset buttons
Add Bypass / Settings / A-B / Listen
Add Auto Gain toggle
Commit
```

## Stage 5 — Meters

```txt
Create LevelMeter component
Create GainReductionMeter component
Add OUT / GR meters
Add placeholder smoothing
Connect to processor meter values later
Commit
```

## Stage 6 — Theme Switching

```txt
Implement Light theme
Implement Dark theme
Add temporary internal theme switch or debug flag
Verify contrast and readability in both themes
Commit
```

## Stage 7 — Polish Pass

```txt
Refine spacing against the fixed coordinates in section 5.1
Refine shadows
Refine text alignment
Refine arc thickness
Check all labels are readable
Remove default JUCE look
Commit
```

---

# 21. Definition of Done

The UI phase is done when:

```txt
Plugin opens at 1100 x 760
Main panels use the fixed coordinates in section 5.1
UI visually matches the Figma reference
Light theme matches the Light Figma frame
Dark theme matches the Dark Figma frame / token board
POLISH is the largest and clearest control
All required controls are visible
All labels are readable
Knobs move smoothly on Windows and macOS
Knobs are real interactive controls, not fake painted-only Components
Meters update smoothly or show placeholders
No default JUCE gray interface remains
No whole-UI screenshot is used
A Git commit is created
```

---

# 22. Final Rule

```txt
POLISH is the hero.
Everything else supports it.
```
