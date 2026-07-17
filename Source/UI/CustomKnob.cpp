#include "CustomKnob.h"

VoxlineCustomKnob::VoxlineCustomKnob(juce::String labelText, juce::Colour, bool isHeroKnob)
    : VoxlineSpriteKnob(std::move(labelText),
                        isHeroKnob ? SpriteSize::large : SpriteSize::autoSize)
{
}
