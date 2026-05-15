#pragma once

#include <JuceHeader.h>

class VoxlineLevelMeter : public juce::Component
{
public:
    VoxlineLevelMeter();

    enum ColourIds
    {
        backgroundColour = 0x1000100,
        foregroundColour = 0x1000101
    };

    void paint(juce::Graphics& g) override;

    void setLevel(float v);

private:
    float level = 0.0f;
    float smoothed = 0.0f;
    float peakHold = 0.0f;
    static constexpr float cornerSize = 4.0f;
    static constexpr int barThickness = 16;
};
