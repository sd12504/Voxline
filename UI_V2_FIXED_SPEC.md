# VOXLINE V2 UI Fixed Layout Specification

Version: 2.0 Concept  
Brand: SADTONY  
Target: Complete Vocal Channel / Complete Vocal Processor  
UI Style: Premium cream vocal plugin, centered POLISH hero control

---

## 0. Important Rules

This document defines the fixed UI layout for VOXLINE V2.

### Hard Rules

- Use fixed absolute coordinates.
- Do not use FlexBox / Grid / proportional scaling for the MVP.
- Do not auto-stretch panels.
- Do not move the POLISH hero control away from the center.
- Do not add a TONE module.
- Do not show BODY / CLARITY / AIR / SMOOTH as main UI controls.
- Vocal EQ handles tone shaping.
- POLISH controls core vocal processing only.
- POLISH must not affect SPACE.
- SPACE is independent.
- All controls must use custom JUCE drawing / custom LookAndFeel.
- No default JUCE look.
- No whole-UI image background.

---

## 1. Editor Size

Use fixed editor size:

```cpp
setSize (1400, 900);
```

Logical coordinate system:

```txt
Editor: x=0 y=0 w=1400 h=900
```

The reference mockup image is 1600×1000.  
Coordinates below are converted and normalized to the 1400×900 JUCE editor.

---

## 2. Global Layout

```txt
Main Window: x=0 y=0 w=1400 h=900
Outer Radius: 24
Background: warm cream
```

Global regions:

```txt
Top Bar:        x=20  y=20  w=1360 h=78
Upper Row:      x=20  y=100 w=1360 h=420
Lower Row:      x=20  y=528 w=1360 h=352
Footer:         x=0   y=872 w=1400 h=24
```

---

## 3. Top Bar

### Top Bar Bounds

```txt
Top Bar: x=20 y=20 w=1360 h=78
```

### Brand Area

```txt
Logo Text:      x=30 y=22 w=260 h=42
Subtitle:       x=32 y=66 w=260 h=20
```

Text:

```txt
Logo: VOXLINE
Subtitle: Complete Vocal Channel
```

Typography:

```txt
Logo: 38px bold, tracking slightly wide
Subtitle: 13px medium
```

### Preset Dropdown

```txt
Preset Label:       x=500 y=28 w=160 h=16
Preset Dropdown:    x=500 y=22 w=260 h=56
Preset Prev Button: x=760 y=22 w=60  h=56
Preset Next Button: x=820 y=22 w=60  h=56
```

Display format:

```txt
PRESET
Clean & Clear
```

or current preset name.

### Utility Buttons

```txt
A/B Button:       x=880  y=28 w=72  h=44
Listen Button:    x=960  y=28 w=128 h=44
Bypass Button:    x=1100 y=28 w=136 h=44
Theme Label:      x=1280 y=30 w=54  h=18
Theme Toggle:     x=1320 y=32 w=64  h=32
Settings Icon:    x=1360 y=32 w=28  h=28
```

Notes:

- A/B can show active slot A or B.
- Listen and Bypass must have clear ON/OFF states.
- Theme toggle supports Light / Dark.
- Settings icon must not be visually louder than Bypass.

---

## 4. Upper Row Panels

Upper row contains:

```txt
Input / Clean Panel
Polish Hero Panel
Output Panel
```

### 4.1 Input / Clean Panel

```txt
InputCleanPanel: x=20 y=105 w=430 h=415
Radius: 18
```

Header:

```txt
Title: x=48 y=122 w=240 h=26
Text: INPUT / CLEAN
```

#### Gain

```txt
Gain Label: x=92 y=168 w=90 h=18
Gain Knob:  x=62 y=205 w=120 h=120
Gain Value: x=70 y=326 w=110 h=24
```

Display:

```txt
GAIN
0.0 dB
```

#### Auto Gain

```txt
AutoGain Label:  x=288 y=168 w=120 h=18
AutoGain Button: x=290 y=202 w=72  h=30
```

Display:

```txt
AUTO GAIN
ON
```

#### Input Dots

```txt
Input Label: x=300 y=258 w=80 h=18
Input Dots:  x=265 y=290 w=135 h=18
Input Value: x=300 y=318 w=90 h=22
```

Display:

```txt
INPUT
-6.2 dB
```

Dots:

```txt
7 dots
dot diameter: 10
dot spacing: 12
inactive opacity: 35%
active colors: green/yellow/amber or purple/rose depending theme
```

#### Divider

```txt
Horizontal Divider: x=48 y=360 w=365 h=1
```

#### Clean Controls

```txt
LowCut Label:  x=70  y=390 w=100 h=18
LowCut Knob:   x=55  y=425 w=78  h=78
LowCut Value:  x=57  y=503 w=80  h=22

Clean Label:   x=210 y=390 w=100 h=18
Clean Knob:    x=195 y=425 w=78  h=78
Clean Value:   x=197 y=503 w=80  h=22

DeEss Label:   x=342 y=390 w=100 h=18
DeEss Knob:    x=327 y=425 w=78  h=78
DeEss Value:   x=329 y=503 w=80  h=22
```

Display:

```txt
LOW CUT  80 Hz
CLEAN    30%
DE-ESS   25%
```

---

### 4.2 Polish Hero Panel

```txt
PolishPanel: x=460 y=105 w=490 h=415
Radius: 18
```

Header:

```txt
Polish Title: x=620 y=128 w=180 h=42
Text: POLISH
```

#### Polish Hero Knob

```txt
Polish Knob:  x=575 y=170 w=270 h=270
Polish Value: x=610 y=430 w=200 h=58
Status Pill:  x=635 y=484 w=150 h=26
Description:  x=525 y=514 w=360 h=42
```

Display:

```txt
65%
PUSHED
VOCAL FINISH MACRO
Controls the core vocal processing intensity.
Does not affect SPACE.
```

Status mapping:

```txt
0–35:   NATURAL
36–70:  PUSHED
71–100: INTENSE
```

Important:

```txt
POLISH affects: Clean / EQ macro / Dynamics / Color / output compensation
POLISH does not affect: spaceAmount / spaceType / slap mix / wide mix / ambience amount
```

---

### 4.3 Output Panel

```txt
OutputPanel: x=960 y=105 w=420 h=415
Radius: 18
```

Header:

```txt
Title: x=990 y=122 w=180 h=26
Text: OUTPUT
```

#### Peak / RMS

```txt
Peak Label:  x=995 y=210 w=65  h=20
Peak Value:  x=995 y=242 w=120 h=36

RMS Label:   x=995 y=322 w=65  h=20
RMS Value:   x=995 y=354 w=130 h=36
```

Display:

```txt
PEAK -3.4 dB
RMS  -14.8 dB
```

Use tabular numbers.

#### Output / GR Meters

```txt
OUT Label:   x=1128 y=168 w=50 h=20
GR Label:    x=1210 y=168 w=50 h=20

OUT Meter:   x=1134 y=206 w=34 h=260
GR Meter:    x=1214 y=206 w=24 h=260
```

Meter style:

```txt
Vertical rounded wells
Continuous fill
Peak hold line
Scale marks
OUT meter visually primary
GR meter visually secondary
```

#### Soft Clip

```txt
SoftClip Button: x=990 y=450 w=105 h=34
```

Display:

```txt
SOFT CLIP
```

#### Output Gain

```txt
OutputGain Label: x=1290 y=260 w=100 h=20
OutputGain Knob:  x=1278 y=305 w=100 h=100
OutputGain Value: x=1280 y=420 w=100 h=24
```

Display:

```txt
OUTPUT GAIN
0.0 dB
```

---

## 5. Lower Row Panels

Lower row contains:

```txt
Vocal EQ Panel
Dynamics / Color Panel
Space Panel
```

### 5.1 Vocal EQ Panel

```txt
VocalEQPanel: x=20 y=530 w=520 h=350
Radius: 18
```

Header:

```txt
Title: x=48 y=548 w=180 h=26
Text: VOCAL EQ
```

EQ enable toggle:

```txt
EQ Toggle: x=470 y=546 w=50 h=24
```

#### EQ Curve Display

```txt
EQ Graph: x=48 y=585 w=465 h=145
```

Graph rules:

```txt
Frequency axis: 20 Hz – 20 kHz, logarithmic
Gain axis: -12 dB – +12 dB
Draw grid lines subtly
Draw total EQ curve
Draw enabled band nodes
First version can be visual-only
```

#### Band Buttons

```txt
Band Row: x=48 y=750 w=465 h=30

HPF Button:  x=48  y=750 w=72 h=30
LOW Button:  x=128 y=750 w=72 h=30
MUD Button:  x=208 y=750 w=72 h=30
PRES Button: x=288 y=750 w=72 h=30
AIR Button:  x=368 y=750 w=72 h=30
LPF Button:  x=448 y=750 w=72 h=30
```

Band color coding:

```txt
HPF: Purple
LOW: Green
MUD: Amber
PRES: Peach
AIR: Blue/Lavender, but avoid harsh blue
LPF: Neutral/Purple
```

#### Selected Band Controls

```txt
Selected Band Label: x=48 y=800 w=160 h=18

Selected Band Button: x=48  y=828 w=80  h=36
Freq Label:           x=145 y=830 w=50  h=18
Freq Knob:            x=188 y=812 w=60  h=60
Freq Value:           x=180 y=870 w=80  h=20

Q/Slope Label:         x=300 y=830 w=70  h=18
Q/Slope Knob:          x=342 y=812 w=60  h=60
Q/Slope Value:         x=330 y=870 w=90  h=20

Reset Button:          x=430 y=828 w=85  h=36
```

Band defaults:

```txt
HPF:      80 Hz, slope 24 dB/oct
LOW:      200 Hz, gain 0 dB, Q 1.0
MUD:      500 Hz, gain 0 dB, Q 1.2
PRES:     3.5 kHz, gain 0 dB, Q 1.2
AIR:      10 kHz, gain 0 dB
LPF:      18 kHz, disabled by default
```

---

### 5.2 Dynamics / Color Panel

```txt
DynamicsColorPanel: x=550 y=530 w=405 h=350
Radius: 18
```

Header:

```txt
Title: x=660 y=548 w=240 h=26
Text: DYNAMICS / COLOR
```

#### Top Row

```txt
Comp Label:  x=610 y=590 w=70 h=18
Comp Knob:   x=595 y=620 w=80 h=80
Comp Value:  x=607 y=704 w=70 h=22

GR Label:    x=735 y=590 w=50 h=18
GR Meter:    x=740 y=620 w=22 h=110

Threshold Label: x=830 y=590 w=105 h=18
Threshold Knob:  x=820 y=620 w=90 h=90
Threshold Value: x=820 y=708 w=100 h=22
```

Display:

```txt
COMP 35%
GR meter
THRESHOLD -18.0 dB
```

#### Divider

```txt
Horizontal Divider: x=590 y=748 w=330 h=1
```

#### Bottom Row

```txt
Ratio Label:   x=585 y=770 w=65 h=18
Ratio Knob:    x=585 y=800 w=55 h=55
Ratio Value:   x=580 y=855 w=70 h=20

Attack Label:  x=675 y=770 w=65 h=18
Attack Knob:   x=675 y=800 w=55 h=55
Attack Value:  x=670 y=855 w=70 h=20

Release Label: x=765 y=770 w=75 h=18
Release Knob:  x=765 y=800 w=55 h=55
Release Value: x=760 y=855 w=80 h=20

Drive Label:   x=880 y=770 w=65 h=18
Drive Knob:    x=865 y=790 w=72 h=72
Drive Value:   x=865 y=858 w=80 h=20
```

Soft Clip:

```txt
SoftClip Button: x=690 y=860 w=110 h=26
```

Note:

```txt
COMP and DRIVE are not part of TONE.
TONE module does not exist in V2.
```

---

### 5.3 Space / Monitor Panel

```txt
SpaceMonitorPanel: x=965 y=530 w=415 h=350
Radius: 18
```

Header:

```txt
Title: x=995 y=548 w=150 h=26
Text: SPACE
```

#### Space Type

```txt
Space Type Selector: x=1100 y=550 w=210 h=38
```

Options:

```txt
Tight
Slap
Wide
```

No Room mode.

#### Space Amount Slider

```txt
Amount Label: x=995 y=620 w=100 h=18
Slider Track: x=995 y=650 w=330 h=20
Slider Value: x=1340 y=635 w=60 h=36
```

Display:

```txt
AMOUNT
24%
```

#### Space Detail Controls

```txt
PreDelay Label: x=1000 y=715 w=90 h=18
PreDelay Knob:  x=1010 y=745 w=65 h=65
PreDelay Value: x=1000 y=812 w=90 h=20

HPF Label: x=1160 y=715 w=90 h=18
HPF Knob:  x=1165 y=745 w=65 h=65
HPF Value: x=1148 y=812 w=110 h=20

LPF Label: x=1320 y=715 w=90 h=18
LPF Knob:  x=1325 y=745 w=65 h=65
LPF Value: x=1310 y=812 w=110 h=20
```

Display:

```txt
PRE-DELAY 15 ms
HPF       200 Hz
LPF       8.0 kHz
```

Important:

```txt
SPACE is independent.
POLISH does not affect SPACE.
```

#### Monitor Section

```txt
Monitor Label: x=995 y=848 w=120 h=20

A/B Button:    x=995  y=875 w=130 h=36
Listen Button: x=1145 y=875 w=130 h=36
Bypass Button: x=1295 y=875 w=130 h=36
```

If these exceed the panel height in implementation, move Monitor row to y=830 and reduce button height to 32.

---

## 6. Footer

```txt
Footer Text: x=0 y=878 w=1400 h=18
Text: VOXLINE 2.0.0  |  SADTONY
```

Typography:

```txt
10–11px
Text muted
Letter spacing
Centered
```

---

## 7. Visual Design Tokens

### Light Theme

```txt
Background:       #F3EFE8
Background 2:     #E6DED3
Main Window:      #FFF9F0
Panel:            #F7F0E7
Panel 2:          #F2E9E0
Panel Border:     #DED3C5

Text Primary:     #26222B
Text Secondary:   #746D7C
Text Muted:       #A198A8

Accent Rose:      #D86F96
Accent Peach:     #E99A5C
Accent Amber:     #D8A548
Accent Purple:    #8D70E8
Accent Lavender:  #B8A6F3
```

### Dark Theme

```txt
Background:       #09080D
Background 2:     #14111D
Main Window:      #111018
Panel:            #181622
Panel 2:          #1E1A29
Panel Border:     #2A2635

Text Primary:     #F5F0EA
Text Secondary:   #9D96A8
Text Muted:       #6E6878

Accent Rose:      #E98BAA
Accent Peach:     #F2A766
Accent Amber:     #E6B45C
Accent Purple:    #A98CFF
Accent Lavender:  #C7B7FF
```

---

## 8. Typography

```txt
Logo:             38px bold
Subtitle:         13px medium
Panel Title:      15–18px semibold uppercase
Section Label:    11–13px semibold uppercase
Knob Label:       11–12px semibold uppercase
Knob Value:       14–18px semibold
POLISH Value:     42–48px bold
Meter Readout:    22–34px bold, tabular figures
Button Text:      12–14px semibold
Footer:           10–11px muted, letter spaced
```

Rules:

```txt
Use tabular numbers for meters and dB values.
Do not use random font weights.
Do not use default JUCE font look.
```

---

## 9. Component Rules

### Knobs

All knobs use:

```txt
Circular body
Subtle inner gradient
Inactive arc
Active arc
Pointer line
Value text
Label text
```

Sizes:

```txt
POLISH:       270 px
Main knobs:   90–120 px
Small knobs:  55–80 px
```

### Buttons

All buttons use:

```txt
Rounded pill
Subtle border
Custom hover / pressed / active states
No default JUCE look
```

### Icons

```txt
SVG path/stroke only
No rect background
No visible bounding boxes
Fixed icon size
6–8 px icon/text spacing
```

### Meters

```txt
Rounded vertical wells
Continuous fill
Peak hold line
Muted silence state
No toy segmented look unless intentionally styled
```

---

## 10. Signal Chain Concept

```txt
Input
→ Input Gain
→ Clean / Low Cut / De-Ess
→ Vocal EQ
→ Dynamics
→ Color / Drive
→ POLISH macro scaling for vocal core processing
→ SPACE
→ Auto Gain
→ Output Gain
→ Bypass Crossfade
→ Soft Clip
→ Output
```

Important:

```txt
POLISH controls vocal core processing.
SPACE controls ambience / slap / width independently.
```

---

## 11. APVTS / Compatibility Rules

Do not delete existing APVTS IDs yet:

```txt
body
clarity
air
smooth
```

They can be hidden/deprecated, but not removed immediately.

Reason:

```txt
Avoid breaking old states
Avoid breaking automation
Avoid breaking preset compatibility
```

V2 visible parameters include:

```txt
inputGain
autoGain
lowCut
cleanAmount
deEss
eqEnabled
eqHpfFreq
eqLowFreq / eqLowGain / eqLowQ
eqMudFreq / eqMudGain / eqMudQ
eqPresenceFreq / eqPresenceGain / eqPresenceQ
eqAirFreq / eqAirGain
eqLpfEnabled / eqLpfFreq
polish
comp
threshold
ratio
attack
release
drive
softClip
spaceAmount
spaceType
spacePreDelay
spaceHpf
spaceLpf
outputGain
bypass
listen
theme
```

---

## 12. Agent Implementation Stages

### Stage 0 — Planning

Create:

```txt
UI_V2_FIXED_SPEC.md
UI_DESIGN_LANGUAGE.md
V2_TODO.md
```

Do not change code.

Commit:

```txt
docs: add VOXLINE V2 fixed UI specification
```

### Stage 1 — Layout Shell

Implement:

```txt
1400×900 editor
TopBar
InputCleanPanel
PolishPanel
OutputPanel
VocalEQPanel
DynamicsColorPanel
SpaceMonitorPanel
Footer
```

Controls can be placeholders first.

Commit:

```txt
feat: add VOXLINE V2 fixed layout shell
```

### Stage 2 — Move Existing Controls

Move existing controls into V2 panels and preserve APVTS attachments.

Commit:

```txt
refactor: move controls into VOXLINE V2 layout
```

### Stage 3 — Vocal EQ UI

Implement EQ curve display and band controls.

Commit:

```txt
feat: add vocal EQ interface
```

### Stage 4 — EQ DSP

Implement 6-band EQ using JUCE DSP IIR filters.

Commit:

```txt
feat: implement vocal EQ DSP
```

### Stage 5 — Space / Dynamics Polish

Finalize Space, Dynamics, Color, A/B, Listen, Bypass.

Commit:

```txt
feat: complete V2 vocal processing modules
```

### Stage 6 — Visual Polish

Apply Light/Dark professional polish.

Commit:

```txt
style: polish VOXLINE V2 professional UI
```

---

## 13. Do Not

- Do not add TONE module.
- Do not show BODY / CLARITY / AIR / SMOOTH in main UI.
- Do not let POLISH affect SPACE.
- Do not add Room reverb.
- Do not use artist preset names.
- Do not use default JUCE LookAndFeel.
- Do not use whole UI image as background.
- Do not change fixed coordinates without approval.
- Do not break APVTS compatibility.
