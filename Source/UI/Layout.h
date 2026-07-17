#pragma once
#include <JuceHeader.h>

namespace VoxlineLayout
{
// === V2 1080×720 Editor ===
static constexpr int editorWidth = 1080;
static constexpr int editorHeight = 720;
static constexpr float panelCornerSize = 14.0f;  // V2 rounded panels

// ===========================================================================
// Top Bar — x=15 y=16 w=1049 h=62
// ===========================================================================
inline const juce::Rectangle<int> topBarBg { 15, 16, 1049, 62 };

inline const juce::Rectangle<int> logoBounds { 23, 18, 201, 34 };
inline const juce::Rectangle<int> subtitleBounds { 25, 53, 201, 16 };

inline const juce::Rectangle<int> presetLabelBounds { 386, 22, 123, 13 };
inline const juce::Rectangle<int> presetDropdownBounds { 386, 18, 201, 45 };
inline const juce::Rectangle<int> presetPrevBounds { 586, 18, 46, 45 };
inline const juce::Rectangle<int> presetNextBounds { 633, 18, 46, 45 };

inline const juce::Rectangle<int> abButtonBounds { 679, 22, 56, 35 };
inline const juce::Rectangle<int> listenUtilityBounds { 741, 22, 99, 35 };
inline const juce::Rectangle<int> bypassToggleBounds { 849, 22, 105, 35 };

// ===========================================================================
// Input / Clean Panel — x=15 y=84 w=332 h=332  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> inputPanel { 15, 84, 332, 332 };
inline const juce::Rectangle<int> inputTitleBounds { 37, 98, 185, 21 };
inline const juce::Rectangle<int> gainLabelBounds { 71, 134, 69, 14 };
inline const juce::Rectangle<int> inputGainKnobBounds { 48, 164, 93, 96 };
inline const juce::Rectangle<int> inputGainValueBounds { 54, 261, 85, 19 };
inline const juce::Rectangle<int> autoGainLabelBounds { 222, 134, 93, 14 };
inline const juce::Rectangle<int> autoGainToggleBounds { 224, 162, 56, 24 };
inline const juce::Rectangle<int> inputLedLabelBounds { 231, 206, 62, 14 };
inline const juce::Rectangle<int> inputLedDotsBounds { 204, 232, 104, 14 };
inline const juce::Rectangle<int> inputLedValueBounds { 231, 254, 69, 18 };
inline const juce::Rectangle<int> inputDividerBounds { 37, 288, 282, 1 };

inline const juce::Rectangle<int> lowCutLabelBounds { 54, 312, 77, 14 };
inline const juce::Rectangle<int> lowCutKnobBounds { 42, 340, 60, 62 };
inline const juce::Rectangle<int> lowCutValueBounds { 44, 402, 62, 18 };
inline const juce::Rectangle<int> cleanLabelBounds { 162, 312, 77, 14 };
inline const juce::Rectangle<int> cleanKnobBounds { 150, 340, 60, 62 };
inline const juce::Rectangle<int> cleanValueBounds { 152, 402, 62, 18 };
inline const juce::Rectangle<int> deEssLabelBounds { 264, 312, 77, 14 };
inline const juce::Rectangle<int> deEssKnobBounds { 252, 340, 60, 62 };
inline const juce::Rectangle<int> deEssValueBounds { 254, 402, 62, 18 };

// ===========================================================================
// POLISH Hero Panel — x=355 y=84 w=378 h=332  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> polishPanel { 355, 84, 378, 332 };
inline const juce::Rectangle<int> polishTitleBounds { 478, 102, 139, 34 };
inline const juce::Rectangle<int> polishSliderBounds { 444, 136, 208, 216 };     // Hero knob
inline const juce::Rectangle<int> polishValueBounds { 471, 344, 154, 46 };
inline const juce::Rectangle<int> polishStatusBounds { 490, 387, 116, 21 };
inline const juce::Rectangle<int> polishDescBounds { 405, 411, 278, 34 };

// ===========================================================================
// Output Panel — x=741 y=84 w=324 h=332  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> outputPanel { 741, 84, 324, 332 };
inline const juce::Rectangle<int> outputTitleBounds { 764, 98, 139, 21 };
inline const juce::Rectangle<int> peakLabelBounds { 768, 168, 50, 16 };
inline const juce::Rectangle<int> peakValueBounds { 768, 194, 93, 29 };
inline const juce::Rectangle<int> rmsLabelBounds { 768, 258, 50, 16 };
inline const juce::Rectangle<int> rmsValueBounds { 768, 283, 100, 29 };
inline const juce::Rectangle<int> outLabelBounds { 870, 134, 39, 16 };
inline const juce::Rectangle<int> grLabelBounds { 933, 134, 39, 16 };
inline const juce::Rectangle<int> outMeterBounds { 875, 165, 26, 208 };
inline const juce::Rectangle<int> grMeterBounds { 937, 165, 19, 208 };
inline const juce::Rectangle<int> softClipBounds { 764, 360, 81, 27 };
inline const juce::Rectangle<int> outputGainLabelBounds { 995, 208, 77, 16 };
inline const juce::Rectangle<int> outputGainKnobBounds { 986, 244, 77, 80 };
inline const juce::Rectangle<int> outputGainValueBounds { 987, 336, 77, 19 };

// ===========================================================================
// Vocal EQ Panel — x=15 y=424 w=401 h=280  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> eqPanel { 15, 424, 401, 280 };
inline const juce::Rectangle<int> eqTitleBounds { 37, 438, 139, 21 };
inline const juce::Rectangle<int> eqOnToggleBounds { 363, 437, 39, 19 };
inline const juce::Rectangle<int> eqCurveBounds { 37, 468, 359, 116 };

inline const juce::Rectangle<int> eqHpfBounds { 37, 600, 56, 24 };
inline const juce::Rectangle<int> eqLowBounds { 99, 600, 56, 24 };
inline const juce::Rectangle<int> eqMudBounds { 160, 600, 56, 24 };
inline const juce::Rectangle<int> eqPresBounds { 222, 600, 56, 24 };
inline const juce::Rectangle<int> eqAirBounds { 284, 600, 56, 24 };
inline const juce::Rectangle<int> eqLpfBounds { 346, 600, 56, 24 };

inline const juce::Rectangle<int> eqSelBandLabelBounds { 37, 640, 123, 14 };
inline const juce::Rectangle<int> eqSelBandBtnBounds { 37, 662, 62, 29 };
inline const juce::Rectangle<int> eqFreqLabelBounds { 112, 664, 39, 14 };
inline const juce::Rectangle<int> eqFreqKnobBounds { 145, 650, 46, 48 };
inline const juce::Rectangle<int> eqFreqValueBounds { 139, 696, 62, 16 };
inline const juce::Rectangle<int> eqQLabelBounds { 231, 664, 54, 14 };
inline const juce::Rectangle<int> eqQKnobBounds { 264, 650, 46, 48 };
inline const juce::Rectangle<int> eqQValueBounds { 255, 696, 69, 16 };
inline const juce::Rectangle<int> eqResetBounds { 332, 662, 66, 29 };
// GAIN/SLOPE knob — second EQ band control (alias positions match Q/Slope column)
inline const juce::Rectangle<int> eqGainLabelBounds { 231, 664, 54, 14 };
inline const juce::Rectangle<int> eqGainKnobBounds { 264, 650, 46, 48 };
inline const juce::Rectangle<int> eqGainValueBounds { 255, 696, 69, 16 };

// ===========================================================================
// Dynamics / Color Panel — x=424 y=424 w=312 h=280  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> dynamicsPanel { 424, 424, 312, 280 };
inline const juce::Rectangle<int> dynamicsTitleBounds { 509, 438, 185, 21 };

inline const juce::Rectangle<int> compLabelBounds { 471, 472, 54, 14 };
inline const juce::Rectangle<int> compKnobBounds { 459, 496, 62, 64 };
inline const juce::Rectangle<int> compValueBounds { 468, 563, 54, 18 };

inline const juce::Rectangle<int> dynamicsGrLabelBounds { 567, 472, 39, 14 };
inline const juce::Rectangle<int> dynamicsGrMeterBounds { 571, 496, 17, 88 };

inline const juce::Rectangle<int> thresholdLabelBounds { 640, 472, 81, 14 };
inline const juce::Rectangle<int> thresholdKnobBounds { 633, 496, 69, 72 };
inline const juce::Rectangle<int> thresholdValueBounds { 633, 566, 77, 18 };

inline const juce::Rectangle<int> dynamicsDividerBounds { 455, 598, 255, 1 };

inline const juce::Rectangle<int> ratioLabelBounds { 451, 616, 50, 14 };
inline const juce::Rectangle<int> ratioKnobBounds { 451, 640, 42, 44 };
inline const juce::Rectangle<int> ratioValueBounds { 447, 684, 54, 16 };

inline const juce::Rectangle<int> attackLabelBounds { 521, 616, 50, 14 };
inline const juce::Rectangle<int> attackKnobBounds { 521, 640, 42, 44 };
inline const juce::Rectangle<int> attackValueBounds { 517, 684, 54, 16 };

inline const juce::Rectangle<int> releaseLabelBounds { 590, 616, 58, 14 };
inline const juce::Rectangle<int> releaseKnobBounds { 590, 640, 42, 44 };
inline const juce::Rectangle<int> releaseValueBounds { 586, 684, 62, 16 };

inline const juce::Rectangle<int> driveLabelBounds { 679, 616, 50, 14 };
inline const juce::Rectangle<int> driveKnobBounds { 667, 632, 56, 58 };
inline const juce::Rectangle<int> driveValueBounds { 667, 686, 62, 16 };

inline const juce::Rectangle<int> dynamicsSoftClipBounds { 532, 688, 85, 21 };

// ===========================================================================
// Space / Monitor Panel — x=744 y=424 w=320 h=280  Radius:14
// ===========================================================================
inline const juce::Rectangle<int> spacePanel { 744, 424, 320, 280 };
inline const juce::Rectangle<int> spaceTitleBounds { 768, 438, 116, 21 };
inline const juce::Rectangle<int> spaceTypeBounds { 849, 440, 162, 30 };
inline const juce::Rectangle<int> spaceAmountLabelBounds { 768, 496, 77, 14 };
inline const juce::Rectangle<int> spaceSliderBounds { 768, 520, 255, 16 };
inline const juce::Rectangle<int> spaceSliderValueBounds { 1034, 508, 46, 29 };
inline const juce::Rectangle<int> spaceValueBounds { 1034, 508, 46, 29 };  // alias for spaceSliderValueBounds

inline const juce::Rectangle<int> spacePreDelayLabelBounds { 771, 572, 69, 14 };
inline const juce::Rectangle<int> spacePreDelayKnobBounds { 779, 596, 50, 52 };
inline const juce::Rectangle<int> spacePreDelayValueBounds { 771, 650, 69, 16 };
inline const juce::Rectangle<int> spaceHpfLabelBounds { 895, 572, 69, 14 };
inline const juce::Rectangle<int> spaceHpfKnobBounds { 899, 596, 50, 52 };
inline const juce::Rectangle<int> spaceHpfValueBounds { 886, 650, 85, 16 };
inline const juce::Rectangle<int> spaceLpfLabelBounds { 1018, 572, 69, 14 };
inline const juce::Rectangle<int> spaceLpfKnobBounds { 1022, 596, 50, 52 };
inline const juce::Rectangle<int> spaceLpfValueBounds { 1011, 650, 85, 16 };

// Monitor section (in space panel)
inline const juce::Rectangle<int> monitorTitleBounds { 768, 678, 93, 16 };
inline const juce::Rectangle<int> monitorAbBounds { 768, 700, 100, 29 };
inline const juce::Rectangle<int> monitorListenBounds { 883, 700, 100, 29 };
inline const juce::Rectangle<int> monitorBypassBounds { 999, 700, 100, 29 };

// ===========================================================================
// Footer — x=0 y=702 w=1080 h=14
// ===========================================================================
inline const juce::Rectangle<int> footerBounds { 0, 702, 1080, 14 };

} // namespace VoxlineLayout
