#include <JuceHeader.h>

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include "../Source/UI/Layout.h"

namespace
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

float averageAbsoluteDifference(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
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

class VoxlineTests final : public juce::UnitTest
{
public:
    VoxlineTests()
        : juce::UnitTest("VOXLINE phases 1 to 4", "VOXLINE")
    {
    }

    void runTest() override
    {
        beginTest("processor exposes the full Phase 1 parameter set");

        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 12);
        }

        beginTest("processor state round-trips parameter values");

        {
            VoxlineAudioProcessor sourceProcessor;
            expectEquals(sourceProcessor.getParameters().size(), 12);

            auto* firstParam = sourceProcessor.getParameters()[0];
            firstParam->setValueNotifyingHost(1.0f);

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            expect(state.getSize() > 0);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto* restoredFirstParam = restoredProcessor.getParameters()[0];
            expectWithinAbsoluteError(restoredFirstParam->getValue(), 1.0f, 0.001f);
        }

        beginTest("editor uses the fixed Phase 4 layout and keeps real controls");

        {
            VoxlineAudioProcessor processor;
            VoxlineAudioProcessorEditor editor(processor);

            expectEquals(editor.getWidth(), VoxlineLayout::editorWidth);
            expectEquals(editor.getHeight(), VoxlineLayout::editorHeight);

            int sliderCount = 0;
            int buttonCount = 0;
            int progressBarCount = 0;
            bool foundPolishSlider = false;
            bool foundListenButton = false;
            bool foundOutputMeter = false;

            for (int i = 0; i < editor.getNumChildComponents(); ++i)
            {
                const auto* child = editor.getChildComponent(i);

                if (const auto* slider = dynamic_cast<const juce::Slider*>(child))
                {
                    ++sliderCount;
                    if (slider->getBounds() == VoxlineLayout::polishSliderBounds)
                        foundPolishSlider = true;
                }

                if (const auto* button = dynamic_cast<const juce::Button*>(child))
                {
                    ++buttonCount;
                    if (button->getBounds() == VoxlineLayout::listenUtilityBounds)
                        foundListenButton = true;
                }

                if (const auto* progressBar = dynamic_cast<const juce::ProgressBar*>(child))
                {
                    ++progressBarCount;
                    if (progressBar->getBounds() == VoxlineLayout::outMeterBounds)
                        foundOutputMeter = true;
                }
            }

            expectEquals(sliderCount, 9);
            expectEquals(buttonCount, 11);
            expectEquals(progressBarCount, 2);
            expect(editor.getLocalBounds().contains(VoxlineLayout::mainCard));
            expect(foundPolishSlider);
            expect(foundListenButton);
            expect(foundOutputMeter);
        }

        beginTest("Phase 3 default DSP audibly changes a vocal-like signal");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            fillTestSignal(buffer, 48000.0);
            juce::AudioBuffer<float> dryBuffer;
            dryBuffer.makeCopyOf(buffer);
            juce::MidiBuffer midi;

            processor.processBlock(buffer, midi);

            expect(averageAbsoluteDifference(buffer, dryBuffer) > 0.01f);
        }

        beginTest("Phase 3 bypass returns the dry signal");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            setBoolParameter(processor, VoxlineParameterIDs::bypass, true);

            juce::AudioBuffer<float> buffer(2, 512);
            fillTestSignal(buffer, 48000.0);
            juce::AudioBuffer<float> dryBuffer;
            dryBuffer.makeCopyOf(buffer);
            juce::MidiBuffer midi;

            processor.processBlock(buffer, midi);

            expectWithinAbsoluteError(averageAbsoluteDifference(buffer, dryBuffer), 0.0f, 1.0e-6f);
        }

        beginTest("Phase 3 output protection contains extreme drive levels");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            setFloatParameter(processor, VoxlineParameterIDs::inputGain, 24.0f);
            setFloatParameter(processor, VoxlineParameterIDs::drive, 100.0f);
            setFloatParameter(processor, VoxlineParameterIDs::outputGain, 24.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample(channel, sample, 2.0f);

            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);

            auto peak = 0.0f;
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

            expect(peak <= 1.0f);
        }
    }
};

static VoxlineTests voxlineTests;
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
