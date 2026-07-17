#include <JuceHeader.h>

#include "../Source/DSP/OutputSafety.h"
#include "../Source/DSP/VocalDrive.h"
#include "../Source/DSP/VocalEq.h"
#include "../Source/PluginProcessor.h"
#include "TestSupport.h"

#include <array>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>

namespace VocalDriveAllocationProbe
{
extern std::atomic<bool> enabled;
extern std::atomic<uint64_t> calls;
}

namespace
{
constexpr auto sampleRate = 48000.0;
constexpr auto blockSize = 512;

void setPlain(VoxlineAudioProcessor& processor,
              const char* parameterId,
              float plainValue)
{
    if (auto* parameter = processor.getAPVTS().getParameter(parameterId))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

void setNeutral(VoxlineAudioProcessor& processor)
{
    setPlain(processor, VoxlineParameterIDs::inputGain, 0.0f);
    setPlain(processor, VoxlineParameterIDs::outputGain, 0.0f);
    setPlain(processor, VoxlineParameterIDs::polish, 0.0f);
    setPlain(processor, VoxlineParameterIDs::smooth, 0.0f);
    setPlain(processor, VoxlineParameterIDs::comp, 0.0f);
    setPlain(processor, VoxlineParameterIDs::drive, 0.0f);
    setPlain(processor, VoxlineParameterIDs::spaceAmount, 0.0f);
    setPlain(processor, VoxlineParameterIDs::eqEnabled, 0.0f);
    setPlain(processor, VoxlineParameterIDs::bypass, 0.0f);
}

void fillSine(juce::AudioBuffer<float>& buffer,
              double rate,
              double frequency,
              float amplitude,
              int64_t startSample)
{
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample(
                channel,
                sample,
                static_cast<float>(
                    amplitude
                    * std::sin(
                        juce::MathConstants<double>::twoPi * frequency
                        * static_cast<double>(startSample + sample) / rate)));
}

float maximumDifference(const juce::AudioBuffer<float>& left,
                        const juce::AudioBuffer<float>& right)
{
    auto difference = 0.0f;
    for (auto channel = 0; channel < left.getNumChannels(); ++channel)
        for (auto sample = 0; sample < left.getNumSamples(); ++sample)
            difference = juce::jmax(
                difference,
                std::abs(left.getSample(channel, sample)
                         - right.getSample(channel, sample)));
    return difference;
}

bool allFinite(const juce::AudioBuffer<float>& buffer)
{
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (!std::isfinite(buffer.getSample(channel, sample)))
                return false;
    return true;
}

void configureChannels(VoxlineAudioProcessor& processor,
                       int channels,
                       juce::UnitTest& test)
{
    auto layout = processor.getBusesLayout();
    const auto channelSet = channels == 1
        ? juce::AudioChannelSet::mono()
        : juce::AudioChannelSet::stereo();
    layout.getChannelSet(true, 0) = channelSet;
    layout.getChannelSet(false, 0) = channelSet;
    test.expect(processor.setBusesLayout(layout));
}

std::vector<float> parameterValues(const VoxlineAudioProcessor& processor)
{
    std::vector<float> values;
    values.reserve(static_cast<size_t>(processor.getParameters().size()));
    for (auto* parameter : processor.getParameters())
        values.push_back(parameter->getValue());
    return values;
}

class ProcessorIntegrationTests final : public juce::UnitTest
{
public:
    ProcessorIntegrationTests()
        : juce::UnitTest("Processor modular integration", "VOXLINE")
    {
    }

    void runTest() override
    {
        beginTest("Input meter is post input gain and pre Vocal EQ");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            setNeutral(processor);
            setPlain(processor, VoxlineParameterIDs::inputGain, 6.0f);
            setPlain(processor, VoxlineParameterIDs::eqEnabled, 1.0f);
            setPlain(processor, VoxlineParameterIDs::clarity, -6.0f);
            setPlain(processor, VoxlineParameterIDs::presFreq, 2500.0f);
            setPlain(processor, VoxlineParameterIDs::presQ, 1.0f);
            processor.prepareToPlay(sampleRate, blockSize);

            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::MidiBuffer midi;
            int64_t position = 0;
            for (auto block = 0; block < 120; ++block)
            {
                fillSine(buffer, sampleRate, 2500.0, 0.25f, position);
                processor.processBlock(buffer, midi);
                position += blockSize;
            }

            const auto input = processor.getInputMeterFrame();
            const auto output = processor.getOutputMeterFrame();
            expectWithinAbsoluteError(
                input.channels[0].peakDbfs,
                -6.0412f,
                0.15f);
            expect(output.channels[0].peakDbfs
                   < input.channels[0].peakDbfs - 2.0f);
        }

        beginTest("Output meter is after output gain clipper and bypass crossfade");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            setNeutral(processor);
            setPlain(processor, VoxlineParameterIDs::inputGain, 24.0f);
            setPlain(processor, VoxlineParameterIDs::outputGain, 24.0f);
            setPlain(processor, VoxlineParameterIDs::bypass, 1.0f);
            processor.prepareToPlay(sampleRate, blockSize);

            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::MidiBuffer midi;
            for (auto block = 0; block < 120; ++block)
            {
                buffer.clear();
                for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                    juce::FloatVectorOperations::fill(
                        buffer.getWritePointer(channel),
                        0.25f,
                        blockSize);
                processor.processBlock(buffer, midi);
            }

            expectWithinAbsoluteError(buffer.getSample(0, blockSize - 1),
                                      0.25f,
                                      1.0e-6f);
            const auto output = processor.getOutputMeterFrame();
            expectWithinAbsoluteError(output.channels[0].peakDbfs,
                                      -12.0412f,
                                      0.15f);
        }

        beginTest("Processor order matches EQ then Drive and not Drive then EQ");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            setNeutral(processor);
            setPlain(processor, VoxlineParameterIDs::eqEnabled, 1.0f);
            setPlain(processor, VoxlineParameterIDs::clarity, 6.0f);
            setPlain(processor, VoxlineParameterIDs::presFreq, 2200.0f);
            setPlain(processor, VoxlineParameterIDs::presQ, 1.2f);
            setPlain(processor, VoxlineParameterIDs::drive, 72.0f);
            setPlain(processor, VoxlineParameterIDs::driveTone, 25.0f);
            setPlain(processor, VoxlineParameterIDs::driveMix, 86.0f);
            setPlain(processor, VoxlineParameterIDs::driveCharacter, 2.0f);
            processor.prepareToPlay(sampleRate, blockSize);

            Voxline::Dsp::ModuleSpec spec {sampleRate, blockSize, 2};
            Voxline::Dsp::VocalEq eqThen;
            Voxline::Dsp::VocalDrive driveThen;
            Voxline::Dsp::VocalEq swappedEq;
            Voxline::Dsp::VocalDrive swappedDrive;
            eqThen.prepare(spec);
            driveThen.prepare(spec);
            swappedEq.prepare(spec);
            swappedDrive.prepare(spec);

            auto eqSettings = Voxline::Dsp::VocalEqSettings {};
            eqSettings.enabled = true;
            eqSettings.low.enabled = true;
            eqSettings.mud.enabled = false;
            eqSettings.presence = {true, 2200.0f, 6.0f, 1.2f};
            eqSettings.air.enabled = true;
            eqSettings.hpf.enabled = false;
            eqSettings.lpf.enabled = false;
            const auto driveSettings = Voxline::Dsp::DriveSettings {
                0.72f,
                Voxline::Dsp::DriveCharacter::edge,
                0.25f,
                0.86f,
                0.0f,
                true};
            eqThen.setTargetSettings(eqSettings);
            swappedEq.setTargetSettings(eqSettings);
            driveThen.setTargetSettings(driveSettings);
            swappedDrive.setTargetSettings(driveSettings);

            juce::AudioBuffer<float> processorBuffer(2, blockSize);
            juce::AudioBuffer<float> expected(2, blockSize);
            juce::AudioBuffer<float> swapped(2, blockSize);
            juce::MidiBuffer midi;
            int64_t position = 0;
            for (auto block = 0; block < 100; ++block)
            {
                fillSine(processorBuffer, sampleRate, 2200.0, 0.23f, position);
                expected.makeCopyOf(processorBuffer);
                swapped.makeCopyOf(processorBuffer);

                processor.processBlock(processorBuffer, midi);
                eqThen.process(expected);
                driveThen.process(expected);
                swappedDrive.process(swapped);
                swappedEq.process(swapped);
                position += blockSize;
            }

            expect(maximumDifference(processorBuffer, expected) < 2.0e-4f);
            expect(maximumDifference(processorBuffer, swapped) > 1.0e-3f);
        }

        beginTest("Polish zero is null and Polish never mutates other parameters");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            setNeutral(processor);
            processor.prepareToPlay(sampleRate, blockSize);

            const auto latency = processor.getLatencySamples();
            std::array<std::deque<float>, 2> delayedDry;
            for (auto& delay : delayedDry)
                delay.assign(static_cast<size_t>(latency), 0.0f);

            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::AudioBuffer<float> expected(2, blockSize);
            juce::MidiBuffer midi;
            int64_t position = 0;
            auto settledDifference = 0.0f;
            for (auto block = 0; block < 120; ++block)
            {
                fillSine(buffer, sampleRate, 997.0, 0.2f, position);
                expected.setSize(2, blockSize, false, false, true);
                for (auto channel = 0; channel < 2; ++channel)
                    for (auto sample = 0; sample < blockSize; ++sample)
                    {
                        delayedDry[static_cast<size_t>(channel)].push_back(
                            buffer.getSample(channel, sample));
                        expected.setSample(
                            channel,
                            sample,
                            delayedDry[static_cast<size_t>(channel)].front());
                        delayedDry[static_cast<size_t>(channel)].pop_front();
                    }

                processor.processBlock(buffer, midi);
                if (block > 100)
                    settledDifference = juce::jmax(
                        settledDifference,
                        maximumDifference(buffer, expected));
                position += blockSize;
            }
            expect(settledDifference < 2.0e-6f);

            const auto before = parameterValues(processor);
            setPlain(processor, VoxlineParameterIDs::polish, 83.0f);
            buffer.clear();
            processor.processBlock(buffer, midi);
            const auto after = parameterValues(processor);
            expectEquals(before.size(), after.size());
            for (size_t index = 0; index < before.size(); ++index)
            {
                const auto* parameter = dynamic_cast<const juce::RangedAudioParameter*>(
                    processor.getParameters()[static_cast<int>(index)]);
                if (parameter != nullptr
                    && parameter->getParameterID() == VoxlineParameterIDs::polish)
                    continue;
                expectEquals(after[index], before[index]);
            }
        }

        beginTest("Retired Auto Gain Clean Mode and global Listen are audio no-ops");
        {
            VoxlineAudioProcessor reference;
            VoxlineAudioProcessor retiredEnabled;
            configureChannels(reference, 2, *this);
            configureChannels(retiredEnabled, 2, *this);
            setNeutral(reference);
            setNeutral(retiredEnabled);
            setPlain(reference, VoxlineParameterIDs::autoGain, 0.0f);
            setPlain(reference, VoxlineParameterIDs::cleanMode, 0.0f);
            setPlain(reference, VoxlineParameterIDs::listen, 0.0f);
            setPlain(retiredEnabled, VoxlineParameterIDs::autoGain, 1.0f);
            setPlain(retiredEnabled, VoxlineParameterIDs::cleanMode, 1.0f);
            setPlain(retiredEnabled, VoxlineParameterIDs::listen, 1.0f);
            reference.prepareToPlay(sampleRate, blockSize);
            retiredEnabled.prepareToPlay(sampleRate, blockSize);

            juce::AudioBuffer<float> referenceBuffer(2, blockSize);
            juce::AudioBuffer<float> retiredBuffer(2, blockSize);
            juce::MidiBuffer midi;
            int64_t position = 0;
            auto difference = 0.0f;
            for (auto block = 0; block < 60; ++block)
            {
                fillSine(referenceBuffer, sampleRate, 3300.0, 0.31f, position);
                retiredBuffer.makeCopyOf(referenceBuffer);
                reference.processBlock(referenceBuffer, midi);
                retiredEnabled.processBlock(retiredBuffer, midi);
                difference = juce::jmax(
                    difference,
                    maximumDifference(referenceBuffer, retiredBuffer));
                position += blockSize;
            }
            expect(difference < 1.0e-7f);
        }

        beginTest("Bypass settles to exact latency aligned dry");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            setNeutral(processor);
            setPlain(processor, VoxlineParameterIDs::polish, 100.0f);
            setPlain(processor, VoxlineParameterIDs::drive, 100.0f);
            processor.prepareToPlay(sampleRate, blockSize);
            setPlain(processor, VoxlineParameterIDs::bypass, 1.0f);

            const auto latency = processor.getLatencySamples();
            std::array<std::deque<float>, 2> delayedDry;
            for (auto& delay : delayedDry)
                delay.assign(static_cast<size_t>(latency), 0.0f);

            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::AudioBuffer<float> expected(2, blockSize);
            juce::MidiBuffer midi;
            int64_t position = 0;
            auto settledDifference = 0.0f;
            for (auto block = 0; block < 80; ++block)
            {
                fillSine(buffer, sampleRate, 713.0, 0.22f, position);
                for (auto channel = 0; channel < 2; ++channel)
                    for (auto sample = 0; sample < blockSize; ++sample)
                    {
                        delayedDry[static_cast<size_t>(channel)].push_back(
                            buffer.getSample(channel, sample));
                        expected.setSample(
                            channel,
                            sample,
                            delayedDry[static_cast<size_t>(channel)].front());
                        delayedDry[static_cast<size_t>(channel)].pop_front();
                    }
                processor.processBlock(buffer, midi);
                if (block > 20)
                    settledDifference = juce::jmax(
                        settledDifference,
                        maximumDifference(buffer, expected));
                position += blockSize;
            }
            expect(settledDifference < 2.0e-6f);
        }

        beginTest("All supported rates channels and block sizes remain finite");
        {
            constexpr std::array<double, 4> rates {
                44100.0, 48000.0, 88200.0, 96000.0};
            constexpr std::array<int, 2> channelCounts {1, 2};
            constexpr std::array<int, 3> blockSizes {64, 512, 2048};

            for (const auto rate : rates)
                for (const auto channels : channelCounts)
                    for (const auto samples : blockSizes)
                    {
                        VoxlineAudioProcessor processor;
                        configureChannels(processor, channels, *this);
                        processor.prepareToPlay(rate, samples);
                        juce::AudioBuffer<float> buffer(channels, samples);
                        VoxlineTest::fillTestSignal(buffer, rate);
                        juce::MidiBuffer midi;
                        processor.processBlock(buffer, midi);
                        expect(allFinite(buffer),
                               "non-finite at "
                                   + juce::String(rate)
                                   + " Hz, "
                                   + juce::String(channels)
                                   + " channels, "
                                   + juce::String(samples)
                                   + " samples");
                    }
        }

        beginTest("Processor reports exactly VocalDrive fixed latency");
        {
            constexpr std::array<double, 4> rates {
                44100.0, 48000.0, 88200.0, 96000.0};
            for (const auto rate : rates)
            {
                VoxlineAudioProcessor processor;
                configureChannels(processor, 2, *this);
                processor.prepareToPlay(rate, blockSize);

                Voxline::Dsp::VocalDrive drive;
                drive.prepare({rate, blockSize, 2});
                expectEquals(processor.getLatencySamples(),
                             drive.latencySamples());
            }
        }

        beginTest("Process never resizes prepared dry or sidechain storage");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            processor.prepareToPlay(sampleRate, 2048);
            const auto before = processor.getPreparedStorageSnapshot();

            juce::AudioBuffer<float> buffer(2, 2048);
            juce::MidiBuffer midi;
            for (auto block = 0; block < 40; ++block)
            {
                VoxlineTest::fillTestSignal(buffer, sampleRate);
                processor.processBlock(buffer, midi);
            }

            const auto after = processor.getPreparedStorageSnapshot();
            expect(before == after);
            expect(before.maximumBlockSize >= 2048);
            expect(before.channels >= 2);
        }

        beginTest("Process performs no allocations after prepare");
        {
            VoxlineAudioProcessor processor;
            configureChannels(processor, 2, *this);
            processor.prepareToPlay(96000.0, 2048);

            juce::AudioBuffer<float> buffer(2, 2048);
            juce::MidiBuffer midi;
            VoxlineTest::fillTestSignal(buffer, 96000.0);
            processor.processBlock(buffer, midi);

            VocalDriveAllocationProbe::calls.store(
                0, std::memory_order_relaxed);
            VocalDriveAllocationProbe::enabled.store(
                true, std::memory_order_release);
            for (auto block = 0; block < 12; ++block)
            {
                VoxlineTest::fillTestSignal(buffer, 96000.0);
                processor.processBlock(buffer, midi);
            }
            VocalDriveAllocationProbe::enabled.store(
                false, std::memory_order_release);

            expectEquals(
                static_cast<int>(
                    VocalDriveAllocationProbe::calls.load(
                        std::memory_order_relaxed)),
                0);
        }
    }
};

ProcessorIntegrationTests processorIntegrationTests;
}
