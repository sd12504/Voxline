#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class VoxlineSpriteKnob : public juce::Slider
{
public:
    enum class SpriteSize
    {
        autoSize,
        small,
        medium,
        large
    };

    VoxlineSpriteKnob(juce::String labelText, SpriteSize size = SpriteSize::autoSize);

    void paint(juce::Graphics& g) override;

    void setTheme(const VoxlineTheme& theme);
    void setSpriteSize(SpriteSize size);
    void setShowInternalLabel(bool show);
    void setShowInternalValue(bool show);

protected:
    juce::Rectangle<float> getKnobBounds() const;
    float getNormalizedValue() const;

private:
    struct SpriteImages
    {
        juce::Image face;
        juce::Image insideStrip;
    };

    SpriteSize resolveSpriteSize() const;
    SpriteImages getImages(SpriteSize size, bool dark) const;
    void drawImageFitted(juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> bounds) const;

    juce::String label;
    SpriteSize spriteSize = SpriteSize::autoSize;
    bool showInternalLabel = true;
    bool showInternalValue = true;
    const VoxlineTheme* theme { nullptr };
};
