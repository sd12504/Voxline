#pragma once

#include "CustomKnob.h"

class VoxlineHeroKnob final : public VoxlineCustomKnob
{
public:
    VoxlineHeroKnob(juce::String labelText, juce::Colour accentColour)
        : VoxlineCustomKnob(std::move(labelText), accentColour, true)
    {
    }
};
