#include "../Source/DSP/VocalDrive.h"

#include <JuceHeader.h>

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;

void fillSine(juce::AudioBuffer<float>& buffer,
              double frequencyHz,
              float amplitude,
              int startSample = 0)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
                         * frequencyHz
                         * static_cast<double>(startSample + sample)
                         / sampleRate;
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
                         int firstSample = 0)
{
    double real {};
    double imaginary {};

    for (int sample = firstSample; sample < static_cast<int>(samples.size()); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
                         * frequencyHz
                         * static_cast<double>(sample)
                         / sampleRate;
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

class VocalDriveTests final : public juce::UnitTest
{
public:
    VocalDriveTests() : juce::UnitTest("Vocal Drive", "VOXLINE") {}

    void runTest() override
    {
        beginTest("Amount zero is an exact null");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 2});

            juce::AudioBuffer<float> audio(2, blockSize);
            fillSine(audio, 997.0, 0.37f);
            juce::AudioBuffer<float> original;
            original.makeCopyOf(audio);

            drive.process(audio);

            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    expectEquals(audio.getSample(channel, sample),
                                 original.getSample(channel, sample));
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

        beginTest("Oversampling suppresses 7 kHz alias bands");
        {
            Voxline::Dsp::VocalDrive drive;
            drive.prepare({sampleRate, blockSize, 1});
            drive.setTargetSettings({0.8f, Voxline::Dsp::DriveCharacter::edge,
                                     0.0f, 1.0f, 0.0f, false});
            const auto output = render(drive, 7000.0, 0.12f, 96000);
            const auto fundamental = componentMagnitude(output, 7000.0, 48000);
            const std::array aliasFrequencies {1000.0, 6000.0, 13000.0, 20000.0};

            float aliasEnergy {};
            for (const auto frequency : aliasFrequencies)
            {
                const auto magnitude = componentMagnitude(output, frequency, 48000);
                aliasEnergy += magnitude * magnitude;
            }

            const auto aliasDb = juce::Decibels::gainToDecibels(
                std::sqrt(aliasEnergy) / juce::jmax(fundamental, 1.0e-8f));
            expect(aliasDb < -30.0f,
                   "Aliased harmonics must remain at least 30 dB below the fundamental");
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
    }
};

VocalDriveTests vocalDriveTests;
}
