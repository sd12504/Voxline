#include "TestSupport.h"

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

namespace VoxlineTest
{
void fillTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto t = static_cast<float>(sample / sampleRate);
            const auto low = 0.22f * std::sin(juce::MathConstants<float>::twoPi * 180.0f * t);
            const auto presence = 0.15f * std::sin(juce::MathConstants<float>::twoPi * 3200.0f * t);
            const auto air = 0.08f * std::sin(juce::MathConstants<float>::twoPi * 10000.0f * t);
            buffer.setSample(channel, sample, low + presence + air);
        }
    }
}

float averageAbsoluteDifference(const juce::AudioBuffer<float>& a,
                                const juce::AudioBuffer<float>& b)
{
    jassert(a.getNumChannels() == b.getNumChannels());
    jassert(a.getNumSamples() == b.getNumSamples());

    auto total = 0.0f;
    auto count = 0;

    for (int channel = 0; channel < a.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < a.getNumSamples(); ++sample)
        {
            total += std::abs(a.getSample(channel, sample) - b.getSample(channel, sample));
            ++count;
        }
    }

    return count > 0 ? total / static_cast<float>(count) : 0.0f;
}

void setFloatParameter(VoxlineAudioProcessor& processor, const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = dynamic_cast<juce::AudioParameterFloat*>(processor.getAPVTS().getParameter(parameterID)))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

void setBoolParameter(VoxlineAudioProcessor& processor, const juce::String& parameterID, bool enabled)
{
    if (auto* parameter = dynamic_cast<juce::AudioParameterBool*>(processor.getAPVTS().getParameter(parameterID)))
        parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
}

int renderEditorScreenshot(const juce::String& outputPath, bool showAdvanced)
{
    VoxlineAudioProcessor processor;
    processor.prepareToPlay(48000.0, 2048);
    juce::AudioBuffer<float> previewAudio(2, 2048);
    juce::MidiBuffer previewMidi;
    for (int pass = 0; pass < 4; ++pass)
    {
        fillTestSignal(previewAudio, 48000.0);
        processor.processBlock(previewAudio, previewMidi);
    }
    VoxlineAudioProcessorEditor editor(processor);

    if (showAdvanced)
        editor.setAdvancedOpen(true);

    juce::Image image(juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics graphics(image);
    editor.paintEntireComponent(graphics, true);

    auto file = juce::File(outputPath).getFullPathName().isEmpty()
                    ? juce::File::getCurrentWorkingDirectory().getChildFile("voxline-editor-screenshot.png")
                    : juce::File(outputPath);

    file.deleteFile();
    juce::PNGImageFormat png;

    if (auto stream = file.createOutputStream())
        if (png.writeImageToStream(image, *stream))
            return 0;

    return 1;
}
}
