#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VoxLookAndFeel.h"

//==============================================================================
class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void resized() override;

private:
    //==============================================================================
    VoxLookAndFeel lookAndFeel;

    juce::Slider rotarySlider;
    juce::Label  valueLabel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
