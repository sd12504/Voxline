#pragma once
#include <JuceHeader.h>

namespace VoxlineLayout
{
static constexpr int editorWidth = 1400;
static constexpr int editorHeight = 900;
static constexpr float panelCornerSize = 18.0f;

// === Top Bar ===
inline const juce::Rectangle<int> logoBounds { 30, 22, 260, 42 };
inline const juce::Rectangle<int> subtitleBounds { 32, 66, 260, 20 };
inline const juce::Rectangle<int> presetDropdownBounds { 500, 24, 260, 52 };
inline const juce::Rectangle<int> abButtonBounds { 880, 28, 72, 44 };
inline const juce::Rectangle<int> listenUtilityBounds { 960, 28, 128, 44 };
inline const juce::Rectangle<int> bypassToggleBounds { 1100, 28, 136, 44 };
inline const juce::Rectangle<int> settingsButtonBounds { 1320, 32, 42, 32 };

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
inline const juce::Rectangle<int> eqCurveBounds { 48, 585, 465, 145 };
inline const juce::Rectangle<int> eqHpfBounds { 48, 750, 72, 30 };
inline const juce::Rectangle<int> eqLowBounds { 128, 750, 72, 30 };
inline const juce::Rectangle<int> eqMudBounds { 208, 750, 72, 30 };
inline const juce::Rectangle<int> eqPresBounds { 288, 750, 72, 30 };
inline const juce::Rectangle<int> eqAirBounds { 368, 750, 72, 30 };
inline const juce::Rectangle<int> eqLpfBounds { 448, 750, 72, 30 };

// === Dynamics ===
inline const juce::Rectangle<int> dynamicsPanel { 550, 530, 405, 350 };
inline const juce::Rectangle<int> dynamicsTitleBounds { 550, 552, 405, 24 };

// Row 1: COMP  |  GR meter  |  THRESHOLD  (equal 60px gaps, 70px knobs)
inline const juce::Rectangle<int> compKnobBounds      { 610, 635, 70, 70 };
inline const juce::Rectangle<int> compLabelBounds     { 610, 616, 70, 16 };
inline const juce::Rectangle<int> compValueBounds     { 610, 708, 70, 16 };

inline const juce::Rectangle<int> dynamicsGrMeterBounds { 740, 635, 22, 100 };
inline const juce::Rectangle<int> dynamicsGrLabelBounds { 732, 738, 38, 14 };

inline const juce::Rectangle<int> thresholdKnobBounds  { 824, 635, 70, 70 };
inline const juce::Rectangle<int> thresholdLabelBounds { 818, 616, 82, 16 };
inline const juce::Rectangle<int> thresholdValueBounds { 818, 708, 82, 16 };

// Row 2: RATIO  |  ATTACK  |  RELEASE  |  DRIVE  (equal 51px gaps, 50px knobs)
inline const juce::Rectangle<int> ratioKnobBounds   { 601, 765, 50, 50 };
inline const juce::Rectangle<int> ratioLabelBounds  { 596, 747, 60, 14 };
inline const juce::Rectangle<int> ratioValueBounds  { 596, 818, 60, 14 };

inline const juce::Rectangle<int> attackKnobBounds  { 702, 765, 50, 50 };
inline const juce::Rectangle<int> attackLabelBounds { 697, 747, 60, 14 };
inline const juce::Rectangle<int> attackValueBounds { 697, 818, 60, 14 };

inline const juce::Rectangle<int> releaseKnobBounds  { 803, 765, 50, 50 };
inline const juce::Rectangle<int> releaseLabelBounds { 798, 747, 60, 14 };
inline const juce::Rectangle<int> releaseValueBounds { 798, 818, 60, 14 };

inline const juce::Rectangle<int> driveKnobBounds   { 904, 765, 50, 50 };
inline const juce::Rectangle<int> driveLabelBounds  { 899, 747, 60, 14 };
inline const juce::Rectangle<int> driveValueBounds  { 899, 818, 60, 14 };

// === Space ===
inline const juce::Rectangle<int> spacePanel { 965, 530, 415, 350 };
inline const juce::Rectangle<int> spaceTitleBounds { 993, 552, 359, 24 };
inline const juce::Rectangle<int> monitorTitleBounds { 993, 840, 359, 24 };
inline const juce::Rectangle<int> spaceTypeBounds { 1100, 550, 210, 38 };
inline const juce::Rectangle<int> spaceSliderBounds { 995, 650, 280, 20 };
inline const juce::Rectangle<int> spaceValueBounds { 1280, 635, 60, 36 };
inline const juce::Rectangle<int> spacePreDelayBounds { 1010, 745, 65, 65 };
inline const juce::Rectangle<int> spaceHpfBounds { 1165, 745, 65, 65 };
inline const juce::Rectangle<int> spaceLpfBounds { 1325, 745, 65, 65 };

// === Monitor ===
inline const juce::Rectangle<int> monitorAbBounds { 995, 875, 130, 36 };
inline const juce::Rectangle<int> monitorListenBounds { 1145, 875, 130, 36 };
inline const juce::Rectangle<int> monitorBypassBounds { 1295, 875, 130, 36 };

// === Footer ===
inline const juce::Rectangle<int> footerBounds { 0, 878, 1400, 18 };
} // namespace VoxlineLayout
