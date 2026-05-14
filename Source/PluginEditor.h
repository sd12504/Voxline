#pragma once

#include <JuceHeader.h>

class VoxlineAudioProcessor;

class VoxlineAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VoxlineAudioProcessorEditor(VoxlineAudioProcessor&);
    ~VoxlineAudioProcessorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    VoxlineAudioProcessor& audioProcessor;
    juce::Label statusLabel;
};

