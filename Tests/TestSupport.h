#pragma once

#include <JuceHeader.h>

class VoxlineAudioProcessor;

namespace VoxlineTest
{
void fillTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate);
float averageAbsoluteDifference(const juce::AudioBuffer<float>& a,
                                const juce::AudioBuffer<float>& b);
void setFloatParameter(VoxlineAudioProcessor& processor,
                       const juce::String& parameterID,
                       float plainValue);
void setBoolParameter(VoxlineAudioProcessor& processor,
                      const juce::String& parameterID,
                      bool enabled);
int renderEditorScreenshot(const juce::String& outputPath, bool showAdvanced);
}
