#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class VoxlineCustomKnob : public juce::Slider
{
public:
    VoxlineCustomKnob(juce::String labelText, juce::Colour accentColour, bool isHeroKnob = false);

    void paint(juce::Graphics& g) override;

    void setTheme(const VoxlineTheme& theme);

private:
    juce::Rectangle<float> getKnobBounds() const;
    float getNormalizedValue() const;

    juce::String label;
    juce::Colour accent;
    bool isHero = false;
    const VoxlineTheme* theme { nullptr };
};
