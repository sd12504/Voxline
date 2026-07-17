#include <JuceHeader.h>

#include "../Source/PluginProcessor.h"
#include "TestSupport.h"

namespace
{
class DspRegressionTests final : public juce::UnitTest
{
public:
    DspRegressionTests()
        : juce::UnitTest("DSP regression", "VOXLINE") {}

    void runTest() override
    {
        beginTest("Phase 3 default DSP audibly changes a vocal-like signal");
        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            VoxlineTest::fillTestSignal(buffer, 48000.0);
            juce::AudioBuffer<float> dryBuffer;
            dryBuffer.makeCopyOf(buffer);
            juce::MidiBuffer midi;

            processor.processBlock(buffer, midi);

            expect(VoxlineTest::averageAbsoluteDifference(buffer, dryBuffer) > 0.01f);
        }

        beginTest("Phase 3 output protection contains extreme drive levels");
        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            VoxlineTest::setFloatParameter(processor, VoxlineParameterIDs::inputGain, 24.0f);
            VoxlineTest::setFloatParameter(processor, VoxlineParameterIDs::drive, 100.0f);
            VoxlineTest::setFloatParameter(processor, VoxlineParameterIDs::outputGain, 24.0f);

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

        beginTest("Advanced de-esser parameters audibly control sibilant content");
        {
            VoxlineAudioProcessor dryProcessor;
            VoxlineAudioProcessor deEssProcessor;
            dryProcessor.prepareToPlay(48000.0, 512);
            deEssProcessor.prepareToPlay(48000.0, 512);

            VoxlineTest::setFloatParameter(dryProcessor, VoxlineParameterIDs::smooth, 0.0f);
            VoxlineTest::setFloatParameter(deEssProcessor, VoxlineParameterIDs::smooth, 100.0f);
            VoxlineTest::setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssFreq, 4000.0f);
            VoxlineTest::setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssThreshold, -40.0f);
            VoxlineTest::setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssRange, 12.0f);
            deEssProcessor.getAPVTS().getParameter(VoxlineParameterIDs::deEssMode)->setValueNotifyingHost(1.0f);

            juce::AudioBuffer<float> dry(2, 512);
            for (int channel = 0; channel < dry.getNumChannels(); ++channel)
                for (int sample = 0; sample < dry.getNumSamples(); ++sample)
                    dry.setSample(channel, sample, 0.3f * std::sin(
                        juce::MathConstants<float>::twoPi * 8000.0f * static_cast<float>(sample) / 48000.0f));

            juce::AudioBuffer<float> treated;
            treated.makeCopyOf(dry);
            juce::MidiBuffer midi;
            dryProcessor.processBlock(dry, midi);
            deEssProcessor.processBlock(treated, midi);

            const auto difference = VoxlineTest::averageAbsoluteDifference(dry, treated);
            expect(difference > 0.001f);
            expect(deEssProcessor.deEssReduction.load() > 1.0f);
        }
    }
};

DspRegressionTests dspRegressionTests;
}
