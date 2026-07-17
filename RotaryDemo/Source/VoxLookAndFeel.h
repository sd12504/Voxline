#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxLookAndFeel();

    //==============================================================================
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxLookAndFeel)
};
