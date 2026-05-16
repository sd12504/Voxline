#pragma once

#include <JuceHeader.h>

namespace VoxlineLayout
{
static constexpr int editorWidth = 1100;
static constexpr int editorHeight = 760;

static constexpr float mainCornerSize = 28.0f;
static constexpr float panelCornerSize = 22.0f;
static constexpr float presetBarCornerSize = 24.0f;

inline const juce::Rectangle<int> mainCard { 34, 36, 1032, 668 };
inline const juce::Rectangle<int> headerArea { 64, 58, 982, 58 };
inline const juce::Rectangle<int> mainPanelRow { 64, 160, 982, 395 };
inline const juce::Rectangle<int> presetBar { 64, 595, 982, 76 };

inline const juce::Rectangle<int> inputPanel { 64, 160, 150, 395 };
inline const juce::Rectangle<int> tonePanel { 230, 160, 370, 395 };
inline const juce::Rectangle<int> polishPanel { 616, 160, 285, 395 };
inline const juce::Rectangle<int> outputPanel { 916, 160, 130, 395 };

inline const juce::Rectangle<int> logoBounds { 64, 62, 230, 34 };
inline const juce::Rectangle<int> subtitleBounds { 66, 96, 180, 18 };
inline const juce::Rectangle<int> bypassButtonBounds { 905, 62, 86, 42 };
inline const juce::Rectangle<int> settingsButtonBounds { 1004, 62, 42, 42 };

inline const juce::Rectangle<int> inputTitleBounds { 64, 186, 150, 24 };
inline const juce::Rectangle<int> inputGainSliderBounds { 89, 241, 100, 160 };
inline const juce::Rectangle<int> autoGainToggleBounds { 82, 425, 112, 24 };
inline const juce::Rectangle<int> cleanModeBounds { 88, 476, 104, 38 };
inline const juce::Rectangle<int> inputLedDotsBounds { 96, 532, 86, 16 };

inline const juce::Rectangle<int> toneTitleBounds { 230, 168, 370, 26 };
inline const juce::Rectangle<int> bodySliderBounds { 245, 205, 100, 160 };
inline const juce::Rectangle<int> claritySliderBounds { 365, 205, 100, 160 };
inline const juce::Rectangle<int> airSliderBounds { 485, 205, 100, 160 };
inline const juce::Rectangle<int> smoothSliderBounds { 245, 345, 100, 160 };
inline const juce::Rectangle<int> compSliderBounds { 365, 345, 100, 160 };
inline const juce::Rectangle<int> driveSliderBounds { 485, 345, 100, 160 };

inline const juce::Rectangle<int> polishTitleBounds { 616, 188, 285, 26 };
inline const juce::Rectangle<int> polishSliderBounds { 662, 214, 193, 260 };
inline const juce::Rectangle<int> spaceTitleBounds { 616, 492, 285, 16 };
inline const juce::Rectangle<int> spaceSliderBounds { 730, 504, 56, 50 };

inline const juce::Rectangle<int> outputTitleBounds { 916, 186, 130, 24 };
inline const juce::Rectangle<int> peakRmsBounds { 936, 216, 88, 44 };
inline const juce::Rectangle<int> outMeterBounds { 958, 270, 18, 165 };
inline const juce::Rectangle<int> grMeterBounds { 990, 270, 18, 165 };
inline const juce::Rectangle<int> meterLabelsBounds { 940, 446, 88, 22 };
inline const juce::Rectangle<int> outputGainSliderBounds { 946, 476, 70, 70 };
inline const juce::Rectangle<int> outputValueBounds { 916, 535, 130, 17 };

inline const juce::Rectangle<int> presetDropdownBounds { 94, 616, 200, 36 };

inline const juce::Rectangle<int> abButtonBounds { 862, 616, 82, 36 };
inline const juce::Rectangle<int> listenUtilityBounds { 958, 616, 82, 36 };
} // namespace VoxlineLayout
