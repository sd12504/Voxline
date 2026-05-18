#pragma once
#include <JuceHeader.h>

namespace VoxlineLayout
{
static constexpr int editorWidth = 1400;
static constexpr int editorHeight = 900;
static constexpr float panelCornerSize = 18.0f;

// === Top Bar ===
inline const juce::Rectangle<int> logoBounds { 30, 28, 260, 42 };
inline const juce::Rectangle<int> subtitleBounds { 32, 72, 260, 20 };
inline const juce::Rectangle<int> presetDropdownBounds { 500, 32, 260, 52 };
inline const juce::Rectangle<int> abButtonBounds { 880, 36, 72, 44 };
inline const juce::Rectangle<int> listenUtilityBounds { 960, 36, 128, 44 };
inline const juce::Rectangle<int> bypassToggleBounds { 1100, 36, 136, 44 };
inline const juce::Rectangle<int> settingsButtonBounds { 1295, 42, 42, 32 };

// === Input / Clean ===
inline const juce::Rectangle<int> inputPanel { 20, 105, 430, 415 };
inline const juce::Rectangle<int> inputTitleBounds { 48, 127, 374, 24 };
inline const juce::Rectangle<int> inputGainKnobBounds { 62, 205, 120, 120 };
inline const juce::Rectangle<int> inputGainValueBounds { 70, 326, 110, 24 };
inline const juce::Rectangle<int> autoGainToggleBounds { 310, 202, 72, 30 };
inline const juce::Rectangle<int> inputLedDotsBounds { 278, 290, 130, 18 };
inline const juce::Rectangle<int> inputDividerBounds { 48, 382, 365, 1 };
inline const juce::Rectangle<int> lowCutKnobBounds { 69, 420, 78, 78 };
inline const juce::Rectangle<int> cleanKnobBounds { 196, 420, 78, 78 };
inline const juce::Rectangle<int> deEssKnobBounds { 323, 420, 78, 78 };

// === POLISH ===
inline const juce::Rectangle<int> polishPanel { 460, 105, 490, 415 };
inline const juce::Rectangle<int> polishTitleBounds { 460, 127, 490, 24 };
inline const juce::Rectangle<int> polishSliderBounds { 575, 170, 270, 270 };
inline const juce::Rectangle<int> polishValueBounds { 610, 430, 200, 58 };
inline const juce::Rectangle<int> polishStatusBounds { 635, 478, 150, 20 };
inline const juce::Rectangle<int> polishDescBounds { 550, 496, 300, 22 };

// === Output ===
inline const juce::Rectangle<int> outputPanel { 960, 105, 420, 415 };
inline const juce::Rectangle<int> outputTitleBounds { 960, 127, 420, 24 };
inline const juce::Rectangle<int> peakLabelBounds { 995, 210, 65, 20 };
inline const juce::Rectangle<int> peakValueBounds { 995, 242, 120, 36 };
inline const juce::Rectangle<int> rmsLabelBounds { 995, 322, 65, 20 };
inline const juce::Rectangle<int> rmsValueBounds { 995, 354, 130, 36 };
inline const juce::Rectangle<int> outMeterBounds { 1134, 206, 34, 260 };
inline const juce::Rectangle<int> grMeterBounds { 1214, 206, 24, 260 };
inline const juce::Rectangle<int> softClipBounds { 990, 450, 105, 34 };
inline const juce::Rectangle<int> outputGainKnobBounds { 1278, 305, 100, 100 };
inline const juce::Rectangle<int> outputGainValueBounds { 1280, 420, 100, 24 };

// === Vocal EQ ===
inline const juce::Rectangle<int> eqPanel { 20, 530, 520, 350 };
inline const juce::Rectangle<int> eqTitleBounds { 48, 552, 464, 24 };
inline const juce::Rectangle<int> eqCurveBounds { 48, 585, 465, 135 };

// Band buttons
inline const juce::Rectangle<int> eqHpfBounds { 48, 726, 72, 28 };
inline const juce::Rectangle<int> eqLowBounds { 128, 726, 72, 28 };
inline const juce::Rectangle<int> eqMudBounds { 208, 726, 72, 28 };
inline const juce::Rectangle<int> eqPresBounds { 288, 726, 72, 28 };
inline const juce::Rectangle<int> eqAirBounds { 368, 726, 72, 28 };
inline const juce::Rectangle<int> eqLpfBounds { 448, 726, 72, 28 };

// Selected band info
inline const juce::Rectangle<int> eqBandLabelBounds { 48, 765, 160, 20 };
inline const juce::Rectangle<int> eqFreqBounds  { 225, 762, 100, 24 };
inline const juce::Rectangle<int> eqSlopeBounds { 345, 762, 108, 24 };
inline const juce::Rectangle<int> eqResetBounds { 478, 765, 44, 22 };

// === Dynamics ===
inline const juce::Rectangle<int> dynamicsPanel { 550, 530, 405, 350 };
inline const juce::Rectangle<int> dynamicsTitleBounds { 550, 552, 405, 24 };

// Row 1: COMP  |  GR meter  |  THRESHOLD  (COMP/THRESHOLD anchored to edges, GR centred in gap)
inline const juce::Rectangle<int> compKnobBounds      { 580, 635, 70, 70 };
inline const juce::Rectangle<int> compLabelBounds     { 580, 616, 70, 16 };
inline const juce::Rectangle<int> compValueBounds     { 580, 708, 70, 16 };

inline const juce::Rectangle<int> dynamicsGrMeterBounds { 732, 620, 22, 100 };
inline const juce::Rectangle<int> dynamicsGrLabelBounds { 724, 722, 38, 14 };

inline const juce::Rectangle<int> thresholdKnobBounds  { 850, 635, 70, 70 };
inline const juce::Rectangle<int> thresholdLabelBounds { 844, 616, 82, 16 };
inline const juce::Rectangle<int> thresholdValueBounds { 844, 708, 82, 16 };

// Row 2: RATIO  |  ATTACK  |  RELEASE  |  DRIVE  (tighter grouping, 50px knobs)
inline const juce::Rectangle<int> ratioKnobBounds   { 578, 775, 50, 50 };
inline const juce::Rectangle<int> ratioLabelBounds  { 573, 758, 60, 14 };
inline const juce::Rectangle<int> ratioValueBounds  { 573, 828, 60, 14 };

inline const juce::Rectangle<int> attackKnobBounds  { 678, 775, 50, 50 };
inline const juce::Rectangle<int> attackLabelBounds { 673, 758, 60, 14 };
inline const juce::Rectangle<int> attackValueBounds { 673, 828, 60, 14 };

inline const juce::Rectangle<int> releaseKnobBounds  { 778, 775, 50, 50 };
inline const juce::Rectangle<int> releaseLabelBounds { 773, 758, 60, 14 };
inline const juce::Rectangle<int> releaseValueBounds { 773, 828, 60, 14 };

inline const juce::Rectangle<int> driveKnobBounds   { 878, 775, 50, 50 };
inline const juce::Rectangle<int> driveLabelBounds  { 873, 758, 60, 14 };
inline const juce::Rectangle<int> driveValueBounds  { 873, 828, 60, 14 };

// === Space ===
inline const juce::Rectangle<int> spacePanel { 965, 530, 415, 350 };
inline const juce::Rectangle<int> spaceTitleBounds { 993, 552, 359, 24 };

// Space type dropdown
inline const juce::Rectangle<int> spaceTypeBounds { 1100, 580, 210, 36 };

// Amount slider row
inline const juce::Rectangle<int> spaceAmountLabelBounds { 993, 630, 70, 16 };
inline const juce::Rectangle<int> spaceSliderBounds { 993, 652, 280, 28 };
inline const juce::Rectangle<int> spaceValueBounds { 1283, 650, 56, 20 };

// Three knobs: PRE-DELAY | HPF | LPF
inline const juce::Rectangle<int> spacePreDelayKnobBounds  { 1045, 720, 50, 50 };
inline const juce::Rectangle<int> spacePreDelayLabelBounds { 1038, 703, 64, 14 };
inline const juce::Rectangle<int> spacePreDelayValueBounds { 1038, 773, 64, 14 };

inline const juce::Rectangle<int> spaceHpfKnobBounds  { 1147, 720, 50, 50 };
inline const juce::Rectangle<int> spaceHpfLabelBounds { 1140, 703, 64, 14 };
inline const juce::Rectangle<int> spaceHpfValueBounds { 1140, 773, 64, 14 };

inline const juce::Rectangle<int> spaceLpfKnobBounds  { 1249, 720, 50, 50 };
inline const juce::Rectangle<int> spaceLpfLabelBounds { 1242, 703, 64, 14 };
inline const juce::Rectangle<int> spaceLpfValueBounds { 1242, 773, 64, 14 };

// Monitor
inline const juce::Rectangle<int> monitorTitleBounds { 993, 820, 359, 24 };
inline const juce::Rectangle<int> monitorAbBounds { 993, 848, 100, 30 };
inline const juce::Rectangle<int> monitorListenBounds { 1110, 848, 100, 30 };
inline const juce::Rectangle<int> monitorBypassBounds { 1227, 848, 100, 30 };

// === Footer ===
inline const juce::Rectangle<int> footerBounds { 0, 878, 1400, 18 };
} // namespace VoxlineLayout
