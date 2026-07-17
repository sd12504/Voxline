#pragma once

#include "SpriteKnob.h"

class VoxlineCustomKnob : public VoxlineSpriteKnob
{
public:
    VoxlineCustomKnob(juce::String labelText, juce::Colour accentColour, bool isHeroKnob = false);
};
