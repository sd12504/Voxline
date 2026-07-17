#include "../Source/DSP/VocalDrive.h"

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

namespace VocalDriveAllocationProbe
{
std::atomic<bool> enabled {false};
std::atomic<uint64_t> calls {};
}

void* operator new(std::size_t size)
{
    if (VocalDriveAllocationProbe::enabled.load(std::memory_order_relaxed))
        VocalDriveAllocationProbe::calls.fetch_add(1, std::memory_order_relaxed);

    if (auto* memory = std::malloc(size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    ::operator delete(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    if (VocalDriveAllocationProbe::enabled.load(std::memory_order_relaxed))
        VocalDriveAllocationProbe::calls.fetch_add(1, std::memory_order_relaxed);

    void* memory {};
    if (posix_memalign(&memory, static_cast<std::size_t>(alignment), size) == 0)
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void operator delete(void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

void fillSine(juce::AudioBuffer<float>& buffer,
              double frequencyHz,
              float amplitude,
              int startSample = 0,
              double renderSampleRate = sampleRate)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
                         * frequencyHz
                         * static_cast<double>(startSample + sample)
                         / renderSampleRate;
        const auto value = amplitude * static_cast<float>(std::sin(phase));

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

float rms(const std::vector<float>& samples, int firstSample = 0)
{
    double sum {};
    for (int sample = firstSample; sample < static_cast<int>(samples.size()); ++sample)
        sum += static_cast<double>(samples[static_cast<size_t>(sample)])
             * samples[static_cast<size_t>(sample)];

    const auto count = juce::jmax(1, static_cast<int>(samples.size()) - firstSample);
    return static_cast<float>(std::sqrt(sum / static_cast<double>(count)));
}

float componentMagnitude(const std::vector<float>& samples,
                         double frequencyHz,
                         int firstSample = 0,
                         double analysisSampleRate = sampleRate)
{
    double real {};
    double imaginary {};

    for (int sample = firstSample; sample < static_cast<int>(samples.size()); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
                         * frequencyHz
                         * static_cast<double>(sample)
                         / analysisSampleRate;
        const auto value = samples[static_cast<size_t>(sample)];
        real += static_cast<double>(value) * std::cos(phase);
        imaginary -= static_cast<double>(value) * std::sin(phase);
    }

    const auto count = juce::jmax(1, static_cast<int>(samples.size()) - firstSample);
    return static_cast<float>(2.0 * std::hypot(real, imaginary)
                              / static_cast<double>(count));
}

std::vector<float> render(Voxline::Dsp::VocalDrive& drive,
                          double frequencyHz,
                          float amplitude,
                          int totalSamples)
{
    std::vector<float> output(static_cast<size_t>(totalSamples));
    juce::AudioBuffer<float> block(1, blockSize);

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const auto samplesThisBlock = juce::jmin(blockSize, totalSamples - offset);
        block.setSize(1, samplesThisBlock, false, false, true);
        fillSine(block, frequencyHz, amplitude, offset);
        drive.process(block);

        for (int sample = 0; sample < samplesThisBlock; ++sample)
            output[static_cast<size_t>(offset + sample)] = block.getSample(0, sample);
    }

    return output;
}

std::vector<float> renderAtSampleRate(Voxline::Dsp::DriveCharacter character,
                                      double renderSampleRate,
                                      double frequencyHz,
                                      float amount)
{
    const auto totalSamples = juce::roundToInt(renderSampleRate);
    constexpr int renderBlockSize = 512;
    Voxline::Dsp::VocalDrive drive;
    drive.prepare({renderSampleRate, renderBlockSize, 1});
    drive.setTargetSettings({amount, character, 0.0f,
                             1.0f, 0.0f, false});

    std::vector<float> output(static_cast<size_t>(totalSamples));
    juce::AudioBuffer<float> block(1, renderBlockSize);
    for (int offset = 0; offset < totalSamples; offset += renderBlockSize)
    {
        const auto samplesThisBlock =
            juce::jmin(renderBlockSize, totalSamples - offset);
        block.setSize(1, samplesThisBlock, false, false, true);
        fillSine(block, frequencyHz, 0.12f, offset, renderSampleRate);
        drive.process(block);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
            output[static_cast<size_t>(offset + sample)] =
                block.getSample(0, sample);
    }
    return output;
}

float foldedFrequency(double frequencyHz, double renderSampleRate)
{
    auto wrapped = std::fmod(frequencyHz, renderSampleRate);
    if (wrapped < 0.0)
        wrapped += renderSampleRate;
    return static_cast<float>(
        wrapped > renderSampleRate * 0.5
            ? renderSampleRate - wrapped
            : wrapped);
}

std::vector<float> renderAutomation(int runtimeBlockSize)
{
    constexpr int totalSamples = 8192;
    Voxline::Dsp::VocalDrive drive;
    drive.prepare({sampleRate, 2048, 1});
    drive.setTargetSettings({0.78f, Voxline::Dsp::DriveCharacter::warm,
                             0.35f, 1.0f, 0.0f, false});
    drive.reset();
    drive.setTargetSettings({0.78f, Voxline::Dsp::DriveCharacter::warm,
                             0.35f, 0.15f, -9.0f, false});

    std::vector<float> output(static_cast<size_t>(totalSamples));
    juce::AudioBuffer<float> block(1, runtimeBlockSize);

    for (int offset = 0; offset < totalSamples; offset += runtimeBlockSize)
    {
        const auto samplesThisBlock =
            juce::jmin(runtimeBlockSize, totalSamples - offset);
        block.setSize(1, samplesThisBlock, false, false, true);
        fillSine(block, 731.0, 0.21f, offset);
        drive.process(block);

        for (int sample = 0; sample < samplesThisBlock; ++sample)
            output[static_cast<size_t>(offset + sample)] =
                block.getSample(0, sample);
    }

    return output;
}

class VocalDriveTests final : public juce::UnitTest
{
public:
    VocalDriveTests() : juce::UnitTest("Vocal Drive", "VOXLINE") {}

    void runTest() override
    {
        beginTest("Amount zero is an exact latency-aligned dry null");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 2});

            constexpr int totalSamples = blockSize * 4;
            std::vector<float> input(static_cast<size_t>(totalSamples));
            std::vector<float> output(static_cast<size_t>(totalSamples));
            juce::AudioBuffer<float> audio(2, blockSize);

            for (int offset = 0; offset < totalSamples; offset += blockSize)
            {
                fillSine(audio, 997.0, 0.37f, offset);
                for (int sample = 0; sample < blockSize; ++sample)
                    input[static_cast<size_t>(offset + sample)] =
                        audio.getSample(0, sample);

                drive.process(audio);

                for (int sample = 0; sample < blockSize; ++sample)
                    output[static_cast<size_t>(offset + sample)] =
                        audio.getSample(0, sample);
            }

            const auto latency = drive.latencySamples();
            expect(latency > 0);
            for (int sample = 0; sample < totalSamples; ++sample)
            {
                const auto expected =
                    sample < latency
                        ? 0.0f
                        : input[static_cast<size_t>(sample - latency)];
                expectEquals(output[static_cast<size_t>(sample)], expected);
            }
        }

        beginTest("Clean Warm and Edge have distinct harmonic ratios");
        {
            std::array<std::array<float, 2>, 3> ratios {};
            const std::array characters {
                Voxline::Dsp::DriveCharacter::clean,
                Voxline::Dsp::DriveCharacter::warm,
                Voxline::Dsp::DriveCharacter::edge
            };

            for (size_t index = 0; index < characters.size(); ++index)
            {
                Voxline::Dsp::VocalDrive drive;
                drive.prepare({sampleRate, blockSize, 1});
                drive.setTargetSettings({0.72f, characters[index], 0.0f,
                                         1.0f, 0.0f, false});
                const auto output = render(drive, 750.0, 0.16f, 16384);
                const auto fundamental = componentMagnitude(output, 750.0, 8192);
                ratios[index][0] = juce::Decibels::gainToDecibels(
                    componentMagnitude(output, 1500.0, 8192)
                    / juce::jmax(fundamental, 1.0e-8f));
                ratios[index][1] = juce::Decibels::gainToDecibels(
                    componentMagnitude(output, 2250.0, 8192)
                    / juce::jmax(fundamental, 1.0e-8f));
            }

            for (size_t left = 0; left < ratios.size(); ++left)
                for (size_t right = left + 1; right < ratios.size(); ++right)
                {
                    const auto distance = std::hypot(
                        ratios[left][0] - ratios[right][0],
                        ratios[left][1] - ratios[right][1]);
                    expect(distance > 2.0f,
                           "Character harmonic fingerprints must be measurably distinct");
                }
        }

        beginTest("Level Match keeps steady-state RMS honest");
        {
            Voxline::Dsp::VocalDrive matched;
            matched.prepare({sampleRate, blockSize, 1});
            matched.setTargetSettings({0.85f, Voxline::Dsp::DriveCharacter::warm,
                                       0.0f, 1.0f, 0.0f, true});
            const auto matchedOutput = render(matched, 1000.0, 0.12f, 48000);

            Voxline::Dsp::VocalDrive unmatched;
            unmatched.prepare({sampleRate, blockSize, 1});
            unmatched.setTargetSettings({0.85f, Voxline::Dsp::DriveCharacter::warm,
                                         0.0f, 1.0f, 0.0f, false});
            const auto unmatchedOutput = render(unmatched, 1000.0, 0.12f, 48000);

            std::vector<float> input(48000);
            for (int sample = 0; sample < static_cast<int>(input.size()); ++sample)
                input[static_cast<size_t>(sample)] =
                    0.12f * static_cast<float>(std::sin(
                        juce::MathConstants<double>::twoPi * 1000.0
                        * static_cast<double>(sample) / sampleRate));

            const auto referenceRms = rms(input, 24000);
            const auto matchedDifference = juce::Decibels::gainToDecibels(
                rms(matchedOutput, 24000) / referenceRms);
            const auto unmatchedDifference = juce::Decibels::gainToDecibels(
                rms(unmatchedOutput, 24000) / referenceRms);

            expect(std::abs(matchedDifference) < 0.5f);
            expect(std::abs(unmatchedDifference) > 0.75f);
        }

        beginTest("Maximum Edge drive remains finite");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 2});
            drive.setTargetSettings({1.0f, Voxline::Dsp::DriveCharacter::edge,
                                     1.0f, 1.0f, 6.0f, false});

            juce::AudioBuffer<float> audio(2, blockSize);
            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    audio.setSample(channel, sample,
                                    sample % 2 == 0 ? 100.0f : -100.0f);

            drive.process(audio);

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    expect(std::isfinite(audio.getSample(channel, sample)));
        }

        beginTest("All characters suppress folded harmonics against a high-rate reference");
        {
            constexpr int referenceFactor = 16;
            constexpr double frequencyHz = 7000.0;
            const std::array characters {
                Voxline::Dsp::DriveCharacter::clean,
                Voxline::Dsp::DriveCharacter::warm,
                Voxline::Dsp::DriveCharacter::edge
            };
            const std::array renderRates {44100.0, 48000.0, 96000.0};

            for (const auto character : characters)
                for (const auto renderRate : renderRates)
                {
                    const auto output =
                        renderAtSampleRate(character, renderRate,
                                           frequencyHz, 0.8f);
                    const auto referenceRate =
                        renderRate * static_cast<double>(referenceFactor);
                    const auto reference =
                        renderAtSampleRate(character, referenceRate,
                                           frequencyHz, 0.8f);
                    const auto outputStart =
                        static_cast<int>(output.size() / 2);
                    const auto referenceStart =
                        static_cast<int>(reference.size() / 2);
                    const auto fundamental =
                        componentMagnitude(output, frequencyHz,
                                           outputStart, renderRate);

                    double residualEnergy {};
                    for (int harmonic = 4; harmonic <= 15; ++harmonic)
                    {
                        const auto harmonicFrequency =
                            frequencyHz * static_cast<double>(harmonic);
                        if (harmonicFrequency <= renderRate * 0.5)
                            continue;

                        const auto alias =
                            foldedFrequency(harmonicFrequency, renderRate);
                        if (alias < 20.0f
                            || alias > static_cast<float>(renderRate * 0.5 - 20.0))
                            continue;

                        const auto outputMagnitude =
                            componentMagnitude(output, alias,
                                               outputStart, renderRate);
                        const auto referenceMagnitude =
                            componentMagnitude(reference, alias,
                                               referenceStart, referenceRate);
                        const auto residual =
                            juce::jmax(0.0f,
                                       outputMagnitude - referenceMagnitude);
                        residualEnergy +=
                            static_cast<double>(residual) * residual;
                    }

                    const auto residualDb =
                        juce::Decibels::gainToDecibels(
                            static_cast<float>(std::sqrt(residualEnergy))
                            / juce::jmax(fundamental, 1.0e-8f));
                    expect(residualDb < -30.0f,
                           "rate=" + juce::String(renderRate)
                               + " character="
                               + juce::String(static_cast<int>(character))
                               + " residual="
                               + juce::String(residualDb, 2) + " dB");
                }
        }

        beginTest("Reported latency equals the measured impulse peak");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 1});
            drive.setTargetSettings({0.7f, Voxline::Dsp::DriveCharacter::clean,
                                     0.0f, 1.0f, 0.0f, false});

            juce::AudioBuffer<float> impulse(1, blockSize);
            impulse.clear();
            impulse.setSample(0, 0, 0.5f);
            drive.process(impulse);

            int peakSample {};
            float peakMagnitude {};
            for (int sample = 0; sample < impulse.getNumSamples(); ++sample)
            {
                const auto magnitude = std::abs(impulse.getSample(0, sample));
                if (magnitude > peakMagnitude)
                {
                    peakMagnitude = magnitude;
                    peakSample = sample;
                }
            }

            expectEquals(peakSample, drive.latencySamples());
            expect(drive.latencySamples() >= 0);
            expect(drive.latencySamples()
                   <= static_cast<int>(std::ceil(sampleRate * 0.0015)));
        }

        beginTest("Parameter automation remains smooth and finite");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 1});
            juce::AudioBuffer<float> audio(1, blockSize);
            fillSine(audio, 500.0, 0.2f);
            drive.process(audio);

            drive.setTargetSettings({1.0f, Voxline::Dsp::DriveCharacter::edge,
                                     1.0f, 1.0f, 6.0f, true});
            fillSine(audio, 500.0, 0.2f, blockSize);
            drive.process(audio);

            float maximumDelta {};
            for (int sample = 1; sample < audio.getNumSamples(); ++sample)
            {
                expect(std::isfinite(audio.getSample(0, sample)));
                maximumDelta = juce::jmax(
                    maximumDelta,
                    std::abs(audio.getSample(0, sample)
                             - audio.getSample(0, sample - 1)));
            }
            expect(maximumDelta < 0.5f);
        }

        beginTest("Mix and Trim automation is invariant to block partitioning");
        {
            const auto blocks64 = renderAutomation(64);
            const auto blocks512 = renderAutomation(512);
            const auto blocks2048 = renderAutomation(2048);

            for (size_t sample = 0; sample < blocks64.size(); ++sample)
            {
                expectWithinAbsoluteError(blocks64[sample],
                                          blocks512[sample],
                                          1.0e-5f);
                expectWithinAbsoluteError(blocks64[sample],
                                          blocks2048[sample],
                                          1.0e-5f);
            }
        }

        beginTest("Level Match toggle begins from the current compensated output");
        {
            Voxline::Dsp::VocalDrive toggled;
            Voxline::Dsp::VocalDrive control;
            toggled.prepare({sampleRate, blockSize, 1});
            control.prepare({sampleRate, blockSize, 1});

            const Voxline::Dsp::DriveSettings matched {
                0.92f, Voxline::Dsp::DriveCharacter::warm,
                0.0f, 1.0f, 0.0f, true
            };
            toggled.setTargetSettings(matched);
            control.setTargetSettings(matched);

            juce::AudioBuffer<float> toggledBlock(1, blockSize);
            juce::AudioBuffer<float> controlBlock(1, blockSize);
            for (int offset = 0; offset < 48000; offset += blockSize)
            {
                fillSine(toggledBlock, 997.0, 0.1f, offset);
                controlBlock.makeCopyOf(toggledBlock);
                toggled.process(toggledBlock);
                control.process(controlBlock);
            }

            auto unmatched = matched;
            unmatched.levelMatch = false;
            toggled.setTargetSettings(unmatched);

            fillSine(toggledBlock, 997.0, 0.1f, 48000);
            controlBlock.makeCopyOf(toggledBlock);
            toggled.process(toggledBlock);
            control.process(controlBlock);

            expectWithinAbsoluteError(toggledBlock.getSample(0, 0),
                                      controlBlock.getSample(0, 0),
                                      1.0e-3f);
            expect(std::abs(toggledBlock.getSample(0, blockSize - 1)
                            - controlBlock.getSample(0, blockSize - 1))
                   > 1.0e-3f);
        }

        beginTest("Process performs no allocations across channel and block sizes");
        {
            for (const auto channels : {1, 2})
                for (const auto runtimeBlockSize : {64, 512, 2048})
                {
                    Voxline::Dsp::VocalDrive drive;
                    drive.prepare({sampleRate, runtimeBlockSize, channels});
                    drive.setTargetSettings({
                        0.85f, Voxline::Dsp::DriveCharacter::edge,
                        0.4f, 0.72f, -2.0f, true
                    });

                    juce::AudioBuffer<float> audio(channels, runtimeBlockSize);
                    fillSine(audio, 997.0, 0.18f);
                    drive.process(audio);

                    VocalDriveAllocationProbe::calls.store(
                        0, std::memory_order_relaxed);
                    VocalDriveAllocationProbe::enabled.store(
                        true, std::memory_order_release);
                    for (int iteration = 0; iteration < 32; ++iteration)
                    {
                        fillSine(audio, 997.0, 0.18f,
                                 iteration * runtimeBlockSize);
                        drive.process(audio);
                    }
                    VocalDriveAllocationProbe::enabled.store(
                        false, std::memory_order_release);

                    expectEquals(
                        static_cast<int>(
                            VocalDriveAllocationProbe::calls.load(
                                std::memory_order_relaxed)),
                        0,
                        "channels=" + juce::String(channels)
                            + " block=" + juce::String(runtimeBlockSize));
                }
        }
    }
};

VocalDriveTests vocalDriveTests;
}
