#include "../Source/DSP/VocalSpace.h"

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace VocalDriveAllocationProbe
{
extern std::atomic<bool> enabled;
extern std::atomic<uint64_t> calls;
}

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

struct StereoRender
{
    std::vector<float> left;
    std::vector<float> right;
};

StereoRender renderStereoImpulse(double renderSampleRate,
                                 int renderBlockSize,
                                 SpaceMode mode,
                                 const SpaceSettings& requested,
                                 int totalSamples)
{
    VocalSpace space;
    space.prepare({renderSampleRate, renderBlockSize, 2});
    auto settings = requested;
    settings.mode = mode;
    space.setTargetSettings(settings);

    StereoRender wet {
        std::vector<float>(static_cast<size_t>(totalSamples)),
        std::vector<float>(static_cast<size_t>(totalSamples))
    };
    juce::AudioBuffer<float> audio(2, renderBlockSize);
    juce::AudioBuffer<float> sidechain(2, renderBlockSize);

    for (int offset = 0; offset < totalSamples; offset += renderBlockSize)
    {
        const auto samplesThisBlock =
            juce::jmin(renderBlockSize, totalSamples - offset);
        audio.clear();
        sidechain.clear();
        if (offset == 0)
        {
            audio.setSample(0, 0, 1.0f);
            audio.setSample(1, 0, 1.0f);
        }

        space.process(audio, sidechain);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
        {
            const auto dry = offset + sample == 0 ? 1.0f : 0.0f;
            wet.left[static_cast<size_t>(offset + sample)] =
                audio.getSample(0, sample) - dry;
            wet.right[static_cast<size_t>(offset + sample)] =
                audio.getSample(1, sample) - dry;
        }
    }

    return wet;
}

float stereoDifferenceEnergy(const StereoRender& render,
                             int startSample = 0)
{
    double energy {};
    for (auto sample = startSample;
         sample < static_cast<int>(render.left.size());
         ++sample)
    {
        const auto difference =
            render.left[static_cast<size_t>(sample)]
            - render.right[static_cast<size_t>(sample)];
        energy += static_cast<double>(difference) * difference;
    }
    return static_cast<float>(energy);
}

float stereoEnergyInRange(const StereoRender& render,
                          int startSample,
                          int endSample)
{
    double energy {};
    const auto boundedStart =
        juce::jlimit(0, static_cast<int>(render.left.size()), startSample);
    const auto boundedEnd =
        juce::jlimit(boundedStart,
                     static_cast<int>(render.left.size()), endSample);
    for (auto sample = boundedStart; sample < boundedEnd; ++sample)
    {
        const auto left = render.left[static_cast<size_t>(sample)];
        const auto right = render.right[static_cast<size_t>(sample)];
        energy += static_cast<double>(left) * left
                + static_cast<double>(right) * right;
    }
    return static_cast<float>(energy);
}

float stereoPeak(const StereoRender& render,
                 int startSample = 0)
{
    float peak {};
    const auto boundedStart =
        juce::jlimit(0, static_cast<int>(render.left.size()), startSample);
    for (auto sample = boundedStart;
         sample < static_cast<int>(render.left.size());
         ++sample)
        peak = juce::jmax(
            peak,
            std::abs(render.left[static_cast<size_t>(sample)]),
            std::abs(render.right[static_cast<size_t>(sample)]));
    return peak;
}

float renderWidthSideRms(double renderSampleRate,
                         int renderBlockSize,
                         float tone)
{
    VocalSpace space;
    space.prepare({renderSampleRate, renderBlockSize, 2});
    SpaceSettings settings;
    settings.amount = 1.0f;
    settings.mode = SpaceMode::width;
    settings.sizeOrTime = 0.8f;
    settings.width = 1.5f;
    settings.tone = tone;
    settings.ducking = 0.0f;
    space.setTargetSettings(settings);

    juce::AudioBuffer<float> audio(2, renderBlockSize);
    juce::AudioBuffer<float> sidechain(2, renderBlockSize);
    double energy {};
    int measuredSamples {};
    const auto totalSamples = juce::roundToInt(renderSampleRate * 0.5);
    const auto warmupSamples = juce::roundToInt(renderSampleRate * 0.25);

    for (int offset = 0; offset < totalSamples; offset += renderBlockSize)
    {
        for (int sample = 0; sample < renderBlockSize; ++sample)
        {
            const auto absoluteSample = offset + sample;
            const auto phase =
                juce::MathConstants<double>::twoPi * 6000.0
                * static_cast<double>(absoluteSample) / renderSampleRate;
            const auto value = 0.2f * static_cast<float>(std::sin(phase));
            audio.setSample(0, sample, value);
            audio.setSample(1, sample, value);
        }
        sidechain.clear();
        space.process(audio, sidechain);

        for (int sample = 0; sample < renderBlockSize; ++sample)
        {
            if (offset + sample < warmupSamples
                || offset + sample >= totalSamples)
                continue;
            const auto side = 0.5f * (audio.getSample(0, sample)
                                      - audio.getSample(1, sample));
            energy += static_cast<double>(side) * side;
            ++measuredSamples;
        }
    }

    return static_cast<float>(
        std::sqrt(energy / static_cast<double>(measuredSamples)));
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

        beginTest("Room has short diffused reflections before its second FDN line");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::room;
            settings.preDelayMs = 0.0f;
            settings.sizeOrTime = 0.62f;
            settings.decaySeconds = 0.4f;
            settings.ducking = 0.0f;

            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::room,
                                    settings, 2048);
            const auto roomSizeScale =
                0.65f + 0.75f * settings.sizeOrTime;
            const auto firstFdnArrival =
                juce::roundToInt(sampleRate * 0.0113 * roomSizeScale);
            const auto secondFdnArrival =
                juce::roundToInt(sampleRate * 0.0179 * roomSizeScale);
            const auto diffusedEnergy =
                stereoEnergyInRange(wet,
                                    firstFdnArrival
                                        + juce::roundToInt(sampleRate * 0.001),
                                    secondFdnArrival
                                        - juce::roundToInt(sampleRate * 0.0005));

            expect(diffusedEnergy > 1.0e-7f,
                   "Room needs short all-pass reflections before the next FDN line arrives");
            expect(stereoDifferenceEnergy(wet, firstFdnArrival) > 1.0e-5f,
                   "The short Room diffuser must preserve mono-to-stereo decorrelation");
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

        beginTest("Room Plate Hall and Slap create width from mono");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.preDelayMs = 0.0f;
            settings.decaySeconds = 1.4f;
            settings.width = 1.5f;
            settings.ducking = 0.0f;

            for (const auto mode : {SpaceMode::room, SpaceMode::plate,
                                    SpaceMode::hall})
            {
                const auto wet =
                    renderStereoImpulse(sampleRate, blockSize, mode,
                                        settings,
                                        juce::roundToInt(sampleRate * 0.75));
                expect(stereoDifferenceEnergy(wet) > 1.0e-5f,
                       "A mono vocal must create decorrelated stereo ambience");
            }

            settings.preDelayMs = 90.0f;
            settings.feedback = 0.5f;
            const auto slap =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::slap,
                                    settings,
                                    juce::roundToInt(sampleRate * 0.5));
            expect(stereoDifferenceEnergy(slap) > 1.0e-5f,
                   "Slap must use distinct left and right taps");
        }

        beginTest("Width mode Tone changes the wet spectrum");
        {
            SpaceSettings dark;
            dark.amount = 1.0f;
            dark.mode = SpaceMode::width;
            dark.sizeOrTime = 0.8f;
            dark.width = 1.5f;
            dark.tone = -1.0f;
            dark.ducking = 0.0f;

            auto bright = dark;
            bright.tone = 1.0f;
            const auto darkWet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::width,
                                    dark, 4096);
            const auto brightWet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::width,
                                    bright, 4096);

            double differenceEnergy {};
            for (int sample = 0; sample < 4096; ++sample)
            {
                const auto difference =
                    darkWet.left[static_cast<size_t>(sample)]
                    - brightWet.left[static_cast<size_t>(sample)];
                differenceEnergy +=
                    static_cast<double>(difference) * difference;
            }
            expect(differenceEnergy > 1.0e-6,
                   "Width Tone must not be a dead parameter");
        }

        beginTest("Tone response is sample-rate and block-size invariant");
        {
            const auto reference =
                renderWidthSideRms(48000.0, 64, -0.7f);
            for (const auto configuration :
                 std::array<std::pair<double, int>, 5> {{
                     {44100.0, 64},
                     {48000.0, 512},
                     {88200.0, 512},
                     {96000.0, 2048},
                     {48000.0, 2048}
                 }})
            {
                const auto measured =
                    renderWidthSideRms(configuration.first,
                                       configuration.second, -0.7f);
                const auto differenceDb =
                    juce::Decibels::gainToDecibels(
                        measured / juce::jmax(1.0e-9f, reference));
                expect(std::abs(differenceDb) < 0.75f,
                       "Physical Tone response must not depend on sample rate or block size");
            }
        }

        beginTest("Pre-delay automation does not sweep across old audio");
        {
            VocalSpace space;
            space.prepare({sampleRate, 64, 2});
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::plate;
            settings.preDelayMs = 200.0f;
            settings.ducking = 0.0f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(2, 64);
            juce::AudioBuffer<float> sidechain(2, 64);
            sidechain.clear();
            for (int block = 0; block < 75; ++block)
            {
                audio.clear();
                for (int sample = 0; sample < 64; ++sample)
                {
                    const auto absoluteSample = block * 64 + sample;
                    if (absoluteSample < 480 || absoluteSample >= 960)
                        continue;
                    const auto phase =
                        juce::MathConstants<double>::twoPi * 4000.0
                        * static_cast<double>(absoluteSample) / sampleRate;
                    const auto value =
                        0.5f * static_cast<float>(std::sin(phase));
                    audio.setSample(0, sample, value);
                    audio.setSample(1, sample, value);
                }
                space.process(audio, sidechain);
            }

            settings.preDelayMs = 50.0f;
            space.setTargetSettings(settings);
            double sweptEnergy {};
            for (int block = 0; block < 16; ++block)
            {
                audio.clear();
                space.process(audio, sidechain);
                for (int channel = 0; channel < 2; ++channel)
                    for (int sample = 0; sample < 64; ++sample)
                    {
                        const auto value = audio.getSample(channel, sample);
                        sweptEnergy += static_cast<double>(value) * value;
                    }
            }
            expect(sweptEnergy < 1.0e-10,
                   "Time changes must crossfade fixed taps instead of sweeping the read head");
        }

        beginTest("Inactive modes cannot resurrect frozen delay contents");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});

            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::slap;
            settings.preDelayMs = 90.0f;
            settings.feedback = 0.9f;
            settings.ducking = 0.0f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(2, blockSize);
            juce::AudioBuffer<float> sidechain(2, blockSize);
            sidechain.clear();
            const auto processSilence = [&] (int samples,
                                             double& measuredEnergy)
            {
                for (int offset = 0; offset < samples; offset += blockSize)
                {
                    audio.clear();
                    space.process(audio, sidechain);
                    for (int sample = 0; sample < blockSize; ++sample)
                    {
                        const auto value = audio.getSample(0, sample);
                        measuredEnergy +=
                            static_cast<double>(value) * value;
                    }
                }
            };

            audio.clear();
            audio.setSample(0, 0, 1.0f);
            audio.setSample(1, 0, 1.0f);
            space.process(audio, sidechain);
            double ignoredEnergy {};
            processSilence(juce::roundToInt(sampleRate * 0.04),
                           ignoredEnergy);

            settings.mode = SpaceMode::width;
            space.setTargetSettings(settings);
            processSilence(juce::roundToInt(sampleRate * 0.5),
                           ignoredEnergy);

            settings.mode = SpaceMode::slap;
            space.setTargetSettings(settings);
            double returnedEnergy {};
            processSilence(juce::roundToInt(sampleRate * 0.14),
                           returnedEnergy);
            expect(returnedEnergy < 1.0e-10,
                   "Returning to a mode must start from reset storage");
        }

        beginTest("Plate tail report contains the complete short-decay diffuser tail");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::plate;
            settings.preDelayMs = 0.0f;
            settings.sizeOrTime = 1.0f;
            settings.decaySeconds = 0.1f;
            settings.tone = 1.0f;
            settings.width = 1.0f;
            settings.ducking = 0.0f;

            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            space.setTargetSettings(settings);
            const auto reportedTail = space.tailSeconds();
            const auto totalSamples = juce::roundToInt(
                sampleRate * (reportedTail + 0.25));
            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::plate,
                                    settings, totalSamples);

            float responsePeak {};
            for (size_t sample = 0; sample < wet.left.size(); ++sample)
                responsePeak =
                    juce::jmax(responsePeak,
                               std::abs(wet.left[sample]),
                               std::abs(wet.right[sample]));

            const auto firstSampleAfterReport =
                juce::jlimit(0, totalSamples,
                             juce::roundToInt(sampleRate * reportedTail) + 1);
            float postReportPeak {};
            for (int sample = firstSampleAfterReport;
                 sample < totalSamples; ++sample)
                postReportPeak =
                    juce::jmax(
                        postReportPeak,
                        std::abs(wet.left[static_cast<size_t>(sample)]),
                        std::abs(wet.right[static_cast<size_t>(sample)]));

            expect(responsePeak > 1.0e-5f);
            expect(postReportPeak <= responsePeak * 0.001f,
                   "No Plate energy above -60 dB may remain after tailSeconds()");
        }

        beginTest("Hall tail report includes the longest modulated FDN line");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::hall;
            settings.preDelayMs = 0.0f;
            settings.sizeOrTime = 1.0f;
            settings.decaySeconds = 0.1f;
            settings.tone = 1.0f;
            settings.width = 1.0f;
            settings.ducking = 0.0f;

            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            space.setTargetSettings(settings);
            const auto reportedTail = space.tailSeconds();
            const auto totalSamples =
                juce::roundToInt(sampleRate * (reportedTail + 0.25));
            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::hall,
                                    settings, totalSamples);
            const auto responsePeak = stereoPeak(wet);
            const auto firstSampleAfterReport =
                juce::roundToInt(sampleRate * reportedTail) + 1;
            const auto postReportPeak =
                stereoPeak(wet, firstSampleAfterReport);

            expect(responsePeak > 1.0e-5f);
            expect(postReportPeak <= responsePeak * 0.001f,
                   "Hall FDN energy above -60 dB must not outlive tailSeconds()");
        }

        beginTest("Width tail report includes dark Tone filter settling");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::width;
            settings.sizeOrTime = 0.0f;
            settings.tone = -1.0f;
            settings.width = 2.0f;
            settings.ducking = 0.0f;

            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            space.setTargetSettings(settings);
            const auto reportedTail = space.tailSeconds();
            const auto totalSamples =
                juce::roundToInt(sampleRate * (reportedTail + 0.05));
            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::width,
                                    settings, totalSamples);
            const auto responsePeak = stereoPeak(wet);
            const auto firstSampleAfterReport =
                juce::roundToInt(sampleRate * reportedTail) + 1;
            const auto postReportPeak =
                stereoPeak(wet, firstSampleAfterReport);

            expect(responsePeak > 1.0e-5f);
            expect(postReportPeak <= responsePeak * 0.001f,
                   "Width Tone energy above -60 dB must not outlive tailSeconds()");
        }

        beginTest("Slap tail report includes dark Tone filter settling");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::slap;
            settings.preDelayMs = 20.0f;
            settings.feedback = 0.0f;
            settings.tone = -1.0f;
            settings.width = 0.0f;
            settings.ducking = 0.0f;

            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            space.setTargetSettings(settings);
            const auto reportedTail = space.tailSeconds();
            const auto totalSamples =
                juce::roundToInt(sampleRate * (reportedTail + 0.05));
            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::slap,
                                    settings, totalSamples);
            const auto responsePeak = stereoPeak(wet);
            const auto firstSampleAfterReport =
                juce::roundToInt(sampleRate * reportedTail) + 1;
            const auto postReportPeak =
                stereoPeak(wet, firstSampleAfterReport);

            expect(responsePeak > 1.0e-5f);
            expect(postReportPeak <= responsePeak * 0.001f,
                   "Slap Tone energy above -60 dB must not outlive tailSeconds()");
        }

        beginTest("Slap feedback tail report includes the unity first repeat");
        {
            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::slap;
            settings.preDelayMs = 80.0f;
            settings.feedback = 0.5f;
            settings.tone = 1.0f;
            settings.width = 0.0f;
            settings.ducking = 0.0f;

            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});
            space.setTargetSettings(settings);
            const auto reportedTail = space.tailSeconds();
            const auto repeatsToMinus60 =
                std::log(0.001) / std::log(
                    static_cast<double>(settings.feedback));
            const auto minimumRepeatTail =
                0.001 * static_cast<double>(settings.preDelayMs)
                * (1.0 + repeatsToMinus60);
            expect(reportedTail >= minimumRepeatTail,
                   "The unity first Slap repeat requires one interval beyond the feedback exponent");

            const auto totalSamples =
                juce::roundToInt(sampleRate * (reportedTail + 0.15));
            const auto wet =
                renderStereoImpulse(sampleRate, blockSize, SpaceMode::slap,
                                    settings, totalSamples);
            const auto responsePeak = stereoPeak(wet);
            const auto firstSampleAfterReport =
                juce::roundToInt(sampleRate * reportedTail) + 1;
            const auto postReportPeak =
                stereoPeak(wet, firstSampleAfterReport);

            expect(responsePeak > 1.0e-5f);
            expect(postReportPeak <= responsePeak * 0.001f,
                   "Feedback Slap energy above -60 dB must not outlive tailSeconds()");
        }

        beginTest("Tail report covers the remaining mode crossfade");
        {
            VocalSpace space;
            space.prepare({sampleRate, blockSize, 2});

            SpaceSettings settings;
            settings.amount = 1.0f;
            settings.mode = SpaceMode::room;
            settings.preDelayMs = 0.0f;
            settings.sizeOrTime = 0.0f;
            settings.decaySeconds = 0.1f;
            settings.ducking = 0.0f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(2, 1);
            juce::AudioBuffer<float> sidechain(2, 1);
            audio.clear();
            sidechain.clear();
            space.process(audio, sidechain);

            settings.mode = SpaceMode::width;
            space.setTargetSettings(settings);
            audio.clear();
            space.process(audio, sidechain);
            expect(space.tailSeconds() >= 0.029,
                   "Transition start must report the old engine's remaining fade");

            const auto midpointSamples =
                juce::roundToInt(sampleRate * 0.015);
            juce::AudioBuffer<float> midpointAudio(2, midpointSamples);
            juce::AudioBuffer<float> midpointSidechain(2, midpointSamples);
            midpointAudio.clear();
            midpointSidechain.clear();
            space.process(midpointAudio, midpointSidechain);
            expect(space.tailSeconds() >= 0.014,
                   "Transition midpoint must report the remaining fade");
        }

        beginTest("Mode transition lasts thirty milliseconds");
        {
            for (const auto transitionSampleRate : {48000.0, 96000.0})
            {
                const auto renderedSamples =
                    juce::roundToInt(transitionSampleRate * 0.035);
                const auto midpoint =
                    juce::roundToInt(transitionSampleRate * 0.015);
                const auto endpoint =
                    juce::roundToInt(transitionSampleRate * 0.030);

                SpaceSettings settings;
                settings.amount = 1.0f;
                settings.mode = SpaceMode::width;
                settings.preDelayMs = 0.0f;
                settings.width = 0.0f;
                settings.ducking = 0.0f;

                VocalSpace switched;
                switched.prepare(
                    {transitionSampleRate, renderedSamples, 1});
                switched.setTargetSettings(settings);
                juce::AudioBuffer<float> prime(1, 64);
                juce::AudioBuffer<float> primeSidechain(1, 64);
                prime.clear();
                primeSidechain.clear();
                switched.process(prime, primeSidechain);

                settings.mode = SpaceMode::room;
                switched.setTargetSettings(settings);
                VocalSpace room;
                room.prepare(
                    {transitionSampleRate, renderedSamples, 1});
                room.setTargetSettings(settings);

                juce::AudioBuffer<float> switchedAudio(
                    1, renderedSamples);
                juce::AudioBuffer<float> roomAudio(
                    1, renderedSamples);
                juce::AudioBuffer<float> sidechain(
                    1, renderedSamples);
                switchedAudio.clear();
                for (int sample = 0; sample < renderedSamples; ++sample)
                    switchedAudio.setSample(0, sample, 0.1f);
                roomAudio.makeCopyOf(switchedAudio);
                sidechain.clear();
                switched.process(switchedAudio, sidechain);
                room.process(roomAudio, sidechain);

                const auto wetRatio = [&] (int sample)
                {
                    const auto switchedWet =
                        switchedAudio.getSample(0, sample) - 0.1f;
                    const auto roomWet =
                        roomAudio.getSample(0, sample) - 0.1f;
                    return switchedWet / roomWet;
                };

                expectWithinAbsoluteError(wetRatio(0), 0.0f, 1.0e-4f);
                expectWithinAbsoluteError(
                    wetRatio(midpoint),
                    std::sqrt(0.5f), 0.015f);
                expectWithinAbsoluteError(
                    wetRatio(endpoint), 1.0f, 1.0e-4f);
            }
        }

        beginTest("Process performs no allocations after prepare");
        {
            VocalSpace space;
            space.prepare({96000.0, 2048, 2});
            SpaceSettings settings;
            settings.amount = 0.8f;
            settings.preDelayMs = 120.0f;
            settings.sizeOrTime = 0.75f;
            settings.decaySeconds = 3.0f;
            settings.width = 1.5f;
            settings.ducking = 0.6f;
            settings.feedback = 0.7f;
            space.setTargetSettings(settings);

            juce::AudioBuffer<float> audio(2, 2048);
            juce::AudioBuffer<float> sidechain(2, 2048);
            fillSine(audio, 0);
            sidechain.makeCopyOf(audio);
            space.process(audio, sidechain);

            VocalDriveAllocationProbe::calls.store(
                0, std::memory_order_relaxed);
            VocalDriveAllocationProbe::enabled.store(
                true, std::memory_order_release);
            for (const auto mode : {SpaceMode::room, SpaceMode::plate,
                                    SpaceMode::hall, SpaceMode::slap,
                                    SpaceMode::width,
                                    SpaceMode::room})
            {
                settings.mode = mode;
                settings.preDelayMs =
                    mode == SpaceMode::slap ? 95.0f : 28.0f;
                space.setTargetSettings(settings);
                for (int block = 0; block < 8; ++block)
                {
                    fillSine(audio, block * 2048);
                    sidechain.makeCopyOf(audio);
                    space.process(audio, sidechain);
                }
            }
            VocalDriveAllocationProbe::enabled.store(
                false, std::memory_order_release);

            expectEquals(
                static_cast<int>(
                    VocalDriveAllocationProbe::calls.load(
                        std::memory_order_relaxed)),
                0);
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
