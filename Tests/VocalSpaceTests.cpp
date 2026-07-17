#include "../Source/DSP/VocalSpace.h"

#include <JuceHeader.h>

#include <array>
#include <cmath>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

using Voxline::Dsp::SpaceMode;
using Voxline::Dsp::SpaceSettings;
using Voxline::Dsp::VocalSpace;

void fillSine(juce::AudioBuffer<float>& buffer,
              int startSample,
              float amplitude = 0.2f)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * 440.0
                         * static_cast<double>(startSample + sample) / sampleRate;
        const auto value = amplitude * static_cast<float>(std::sin(phase));
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

float rms(const std::vector<float>& samples, int start, int count)
{
    double energy {};
    for (int index = start; index < start + count; ++index)
        energy += static_cast<double>(samples[static_cast<size_t>(index)])
                * samples[static_cast<size_t>(index)];
    return static_cast<float>(std::sqrt(energy / static_cast<double>(count)));
}

std::vector<float> renderImpulse(SpaceMode mode,
                                 const SpaceSettings& requested,
                                 int totalSamples)
{
    VocalSpace space;
    space.prepare({sampleRate, blockSize, 1});
    auto settings = requested;
    settings.mode = mode;
    space.setTargetSettings(settings);

    std::vector<float> wet(static_cast<size_t>(totalSamples));
    juce::AudioBuffer<float> audio(1, blockSize);
    juce::AudioBuffer<float> sidechain(1, blockSize);

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const auto samplesThisBlock = juce::jmin(blockSize, totalSamples - offset);
        audio.setSize(1, samplesThisBlock, false, false, true);
        sidechain.setSize(1, samplesThisBlock, false, false, true);
        audio.clear();
        sidechain.clear();
        if (offset == 0)
            audio.setSample(0, 0, 1.0f);

        space.process(audio, sidechain);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
        {
            const auto dry = offset + sample == 0 ? 1.0f : 0.0f;
            wet[static_cast<size_t>(offset + sample)] =
                audio.getSample(0, sample) - dry;
        }
    }

    return wet;
}

int firstNonZero(const std::vector<float>& samples, float threshold = 1.0e-6f)
{
    for (int sample = 0; sample < static_cast<int>(samples.size()); ++sample)
        if (std::abs(samples[static_cast<size_t>(sample)]) > threshold)
            return sample;
    return -1;
}

class VocalSpaceTests final : public juce::UnitTest
{
public:
    VocalSpaceTests() : juce::UnitTest("Vocal Space", "VOXLINE") {}

    void runTest() override
    {
        beginTest("Amount zero is an exact null");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            juce::AudioBuffer<float> audio(2, blockSize);
            juce::AudioBuffer<float> sidechain(2, blockSize);
            fillSine(audio, 0);
            sidechain.makeCopyOf(audio);
            juce::AudioBuffer<float> original;
            original.makeCopyOf(audio);

            space.process(audio, sidechain);

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    expectEquals(audio.getSample(channel, sample),
                                 original.getSample(channel, sample));
        }

        beginTest("Pre-delay places the first wet sample within one sample");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.preDelayMs = 28.0f;
            settings.ducking = 0.0f;
            const auto wet = renderImpulse(SpaceMode::plate, settings, 4096);
            const auto expected = juce::roundToInt(sampleRate
                                                   * settings.preDelayMs
                                                   * 0.001);
            expect(std::abs(firstNonZero(wet) - expected) <= 1);
        }

        beginTest("Room Plate and Hall expose distinct tail structures");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.preDelayMs = 0.0f;
            settings.decaySeconds = 1.2f;
            settings.ducking = 0.0f;

            VocalSpace room;
            room.prepare({sampleRate, blockSize, 2});
            settings.mode = SpaceMode::room;
            room.setTargetSettings(settings);

            VocalSpace plate;
            plate.prepare({sampleRate, blockSize, 2});
            settings.mode = SpaceMode::plate;
            plate.setTargetSettings(settings);

            VocalSpace hall;
            hall.prepare({sampleRate, blockSize, 2});
            settings.mode = SpaceMode::hall;
            hall.setTargetSettings(settings);

            expect(room.tailSeconds() < plate.tailSeconds());
            expect(plate.tailSeconds() < hall.tailSeconds());

            const auto roomWet = renderImpulse(SpaceMode::room, settings, 4800);
            const auto plateWet = renderImpulse(SpaceMode::plate, settings, 4800);
            int roomEarlySamples {};
            int plateEarlySamples {};
            for (int sample = 1; sample < 3600; ++sample)
            {
                roomEarlySamples += std::abs(roomWet[static_cast<size_t>(sample)])
                                    > 1.0e-5f;
                plateEarlySamples += std::abs(plateWet[static_cast<size_t>(sample)])
                                     > 1.0e-5f;
            }
            expect(plateEarlySamples > roomEarlySamples,
                   "Plate must produce more populated early reflections than Room");
        }

        beginTest("Slap repeat follows Time and feedback decays");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::slap;
            settings.preDelayMs = 90.0f;
            settings.feedback = 0.45f;
            settings.ducking = 0.0f;
            const auto wet = renderImpulse(SpaceMode::slap, settings, 15000);
            const auto delay = juce::roundToInt(sampleRate
                                                * settings.preDelayMs
                                                * 0.001);
            expect(std::abs(firstNonZero(wet) - delay) <= 1);
            expect(std::abs(wet[static_cast<size_t>(delay * 2)])
                   < std::abs(wet[static_cast<size_t>(delay)]));
            expect(std::abs(wet[static_cast<size_t>(delay * 3)])
                   < std::abs(wet[static_cast<size_t>(delay * 2)]));
        }

        beginTest("Width adds stereo difference with mono-safe fold-down");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::width;
            settings.sizeOrTime = 1.0f;
            settings.width = 2.0f;
            settings.ducking = 0.0f;
            settings.monoSafety = true;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(2, blockSize);
            juce::AudioBuffer<float> sidechain(2, blockSize);
            float differenceEnergy {};
            float foldEnergy {};
            float dryEnergy {};

            for (int block = 0; block < 16; ++block)
            {
                fillSine(audio, block * blockSize);
                sidechain.clear();
                for (int sample = 0; sample < blockSize; ++sample)
                {
                    const auto dry = audio.getSample(0, sample);
                    dryEnergy += dry * dry;
                }
                space.process(audio, sidechain);

                for (int sample = 0; sample < blockSize; ++sample)
                {
                    const auto left = audio.getSample(0, sample);
                    const auto right = audio.getSample(1, sample);
                    const auto difference = left - right;
                    const auto fold = 0.5f * (left + right);
                    differenceEnergy += difference * difference;
                    foldEnergy += fold * fold;
                }
            }

            expect(differenceEnergy > 0.01f);
            const auto foldDifferenceDb = juce::Decibels::gainToDecibels(
                std::sqrt(foldEnergy / dryEnergy));
            expect(std::abs(foldDifferenceDb) < 1.0f);
        }

        beginTest("Ducking lowers wet voice and releases afterwards");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 1});
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::plate;
            settings.preDelayMs = 0.0f;
            settings.ducking = 1.0f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(1, blockSize);
            juce::AudioBuffer<float> sidechain(1, blockSize);
            std::vector<float> voicedWet(blockSize);
            std::vector<float> releasedWet(blockSize);

            for (int block = 0; block < 40; ++block)
            {
                fillSine(audio, block * blockSize);
                sidechain.clear();
                for (int sample = 0; sample < blockSize; ++sample)
                    sidechain.setSample(0, sample, 0.8f);
                juce::AudioBuffer<float> dry;
                dry.makeCopyOf(audio);
                space.process(audio, sidechain);
                if (block == 39)
                    for (int sample = 0; sample < blockSize; ++sample)
                        voicedWet[static_cast<size_t>(sample)] =
                            audio.getSample(0, sample) - dry.getSample(0, sample);
            }

            for (int block = 0; block < 80; ++block)
            {
                fillSine(audio, (40 + block) * blockSize);
                sidechain.clear();
                juce::AudioBuffer<float> dry;
                dry.makeCopyOf(audio);
                space.process(audio, sidechain);
                if (block == 79)
                    for (int sample = 0; sample < blockSize; ++sample)
                        releasedWet[static_cast<size_t>(sample)] =
                            audio.getSample(0, sample) - dry.getSample(0, sample);
            }

            expect(rms(releasedWet, 0, blockSize)
                   > rms(voicedWet, 0, blockSize) * 1.5f);
        }

        beginTest("Mode changes crossfade without a discontinuity");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 1});
            SpaceSettings settings;
            settings.amount = 0.8f;
            settings.mode = SpaceMode::room;
            settings.preDelayMs = 0.0f;
            settings.ducking = 0.0f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(1, blockSize);
            juce::AudioBuffer<float> sidechain(1, blockSize);
            for (int block = 0; block < 20; ++block)
            {
                fillSine(audio, block * blockSize);
                sidechain.clear();
                space.process(audio, sidechain);
            }

            settings.mode = SpaceMode::hall;
            space.setTargetSettings(settings);
            fillSine(audio, 20 * blockSize);
            sidechain.clear();
            space.process(audio, sidechain);

            float maximumDelta {};
            for (int sample = 1; sample < blockSize; ++sample)
            {
                expect(std::isfinite(audio.getSample(0, sample)));
                maximumDelta = juce::jmax(
                    maximumDelta,
                    std::abs(audio.getSample(0, sample)
                             - audio.getSample(0, sample - 1)));
            }
            expect(maximumDelta < 0.5f);
        }

        beginTest("Prepared storage handles repeated process calls");
        {
            VocalSpace space;
            space.prepare({sampleRate, 2048, 2});
            SpaceSettings settings;
            settings.amount = 0.5f;
            space.setTargetSettings(settings);

            for (const auto samples : std::array {64, 512, 2048, 64})
            {
                juce::AudioBuffer<float> audio(2, samples);
                juce::AudioBuffer<float> sidechain(2, samples);
                fillSine(audio, 0);
                sidechain.clear();
                auto* const leftStorage = audio.getWritePointer(0);
                auto* const rightStorage = audio.getWritePointer(1);
                space.process(audio, sidechain);
                expect(audio.getWritePointer(0) == leftStorage);
                expect(audio.getWritePointer(1) == rightStorage);
            }
        }
    }
};

VocalSpaceTests vocalSpaceTests;
}
