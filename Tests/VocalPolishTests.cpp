#include <JuceHeader.h>

#include "../Source/DSP/VocalPolish.h"

#include <cmath>
#include <type_traits>

namespace
{
using Voxline::Dsp::ModuleSpec;
using Voxline::Dsp::PolishSettings;
using Voxline::Dsp::VocalPolish;

constexpr int testBlockSize = 128;

float rmsOf(const juce::AudioBuffer<float>& buffer, int startSample = 0) noexcept
{
    double sumSquares = 0.0;
    const auto sampleCount = buffer.getNumSamples() - startSample;

    for (int sample = startSample; sample < buffer.getNumSamples(); ++sample)
        sumSquares += static_cast<double>(buffer.getSample(0, sample))
                    * static_cast<double>(buffer.getSample(0, sample));

    return sampleCount > 0
               ? static_cast<float>(std::sqrt(sumSquares / sampleCount))
               : 0.0f;
}

float peakOf(const juce::AudioBuffer<float>& buffer, int startSample = 0) noexcept
{
    float peak = 0.0f;

    for (int sample = startSample; sample < buffer.getNumSamples(); ++sample)
        peak = juce::jmax(peak, std::abs(buffer.getSample(0, sample)));

    return peak;
}

void fillVocalLikeBlock(juce::AudioBuffer<float>& buffer,
                        double sampleRate,
                        int64_t firstSample) noexcept
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto absoluteSample = firstSample + sample;
        const auto time = static_cast<double>(absoluteSample) / sampleRate;
        const auto transientPhase = absoluteSample
                                    % static_cast<int64_t>(sampleRate * 0.125);
        const auto transient = transientPhase < 18
                                   ? 0.28f
                                         * std::exp(-0.16f
                                                    * static_cast<float>(
                                                        transientPhase))
                                   : 0.0f;
        const auto value =
            0.18f * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi
                                               * 180.0 * time))
            + 0.07f
                  * static_cast<float>(std::sin(
                      juce::MathConstants<double>::twoPi * 760.0 * time))
            + 0.025f
                  * static_cast<float>(std::sin(
                      juce::MathConstants<double>::twoPi * 3400.0 * time))
            + transient;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

struct PolishMeasurement
{
    float inputRms {};
    float outputRms {};
    float inputCrest {};
    float outputCrest {};
};

PolishMeasurement renderVocalLike(double sampleRate, float amount)
{
    VocalPolish polish;
    polish.prepare({sampleRate, testBlockSize, 1});
    polish.setTargetSettings({amount});

    const auto totalSamples = static_cast<int>(sampleRate * 2.0);
    const auto measurementStart = static_cast<int>(sampleRate);
    juce::AudioBuffer<float> inputCapture(1, totalSamples - measurementStart);
    juce::AudioBuffer<float> outputCapture(1, totalSamples - measurementStart);
    juce::AudioBuffer<float> block(1, testBlockSize);
    int captureWrite = 0;

    for (int firstSample = 0; firstSample < totalSamples;
         firstSample += testBlockSize)
    {
        const auto samplesThisBlock =
            juce::jmin(testBlockSize, totalSamples - firstSample);
        block.setSize(1, samplesThisBlock, false, false, true);
        fillVocalLikeBlock(block, sampleRate, firstSample);

        if (firstSample >= measurementStart)
            inputCapture.copyFrom(0, captureWrite, block, 0, 0,
                                  samplesThisBlock);

        polish.process(block);

        if (firstSample >= measurementStart)
        {
            outputCapture.copyFrom(0, captureWrite, block, 0, 0,
                                   samplesThisBlock);
            captureWrite += samplesThisBlock;
        }
    }

    return {rmsOf(inputCapture),
            rmsOf(outputCapture),
            peakOf(inputCapture) / juce::jmax(1.0e-9f, rmsOf(inputCapture)),
            peakOf(outputCapture) / juce::jmax(1.0e-9f, rmsOf(outputCapture))};
}

float renderSineGainDb(double sampleRate, double frequency)
{
    VocalPolish polish;
    polish.prepare({sampleRate, testBlockSize, 1});
    polish.setTargetSettings({0.5f});

    const auto totalSamples = static_cast<int>(sampleRate * 1.5);
    const auto measurementStart = static_cast<int>(sampleRate * 0.5);
    juce::AudioBuffer<float> block(1, testBlockSize);
    double inputSquares = 0.0;
    double outputSquares = 0.0;
    int measuredSamples = 0;

    for (int firstSample = 0; firstSample < totalSamples;
         firstSample += testBlockSize)
    {
        const auto samplesThisBlock =
            juce::jmin(testBlockSize, totalSamples - firstSample);
        block.setSize(1, samplesThisBlock, false, false, true);

        for (int sample = 0; sample < samplesThisBlock; ++sample)
        {
            const auto absoluteSample = firstSample + sample;
            block.setSample(
                0, sample,
                0.2f
                    * static_cast<float>(
                        std::sin(juce::MathConstants<double>::twoPi * frequency
                                 * absoluteSample / sampleRate)));
        }

        if (firstSample >= measurementStart)
            for (int sample = 0; sample < samplesThisBlock; ++sample)
            {
                const auto value = block.getSample(0, sample);
                inputSquares += static_cast<double>(value) * value;
            }

        polish.process(block);

        if (firstSample >= measurementStart)
            for (int sample = 0; sample < samplesThisBlock; ++sample)
            {
                const auto value = block.getSample(0, sample);
                outputSquares += static_cast<double>(value) * value;
                ++measuredSamples;
            }
    }

    const auto inputRms =
        static_cast<float>(std::sqrt(inputSquares / measuredSamples));
    const auto outputRms =
        static_cast<float>(std::sqrt(outputSquares / measuredSamples));
    return juce::Decibels::gainToDecibels(outputRms
                                          / juce::jmax(1.0e-9f, inputRms));
}

class VocalPolishTests final : public juce::UnitTest
{
public:
    VocalPolishTests() : juce::UnitTest("Vocal Polish", "VOXLINE") {}

    void runTest() override
    {
        beginTest("zero amount is sample-for-sample null");
        {
            VocalPolish polish;
            polish.prepare({48000.0, testBlockSize, 2});
            polish.setTargetSettings({0.0f});
            juce::AudioBuffer<float> buffer(2, testBlockSize);

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample(
                        channel, sample,
                        0.37f
                            * static_cast<float>(std::sin(
                                0.031 * (sample + 17 * channel))));

            juce::AudioBuffer<float> expected;
            expected.makeCopyOf(buffer);
            polish.process(buffer);

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    expectEquals(buffer.getSample(channel, sample),
                                 expected.getSample(channel, sample));
        }

        beginTest("amount is independent and does not mutate caller state");
        {
            VocalPolish polish;
            polish.prepare({48000.0, testBlockSize, 1});
            PolishSettings externalSettings {0.73f};
            const PolishSettings originalSettings = externalSettings;
            float unrelatedExternalParameter = -4.25f;
            polish.setTargetSettings(externalSettings);
            juce::AudioBuffer<float> buffer(1, testBlockSize);
            buffer.clear();
            polish.process(buffer);

            expectEquals(externalSettings.amount, originalSettings.amount);
            expectEquals(unrelatedExternalParameter, -4.25f);
            expect(std::is_nothrow_invocable_v<
                   decltype(&VocalPolish::process),
                   VocalPolish*,
                   juce::AudioBuffer<float>&>);
        }

        beginTest("50 percent adds density without a loudness jump");
        {
            const auto dry = renderVocalLike(48000.0, 0.0f);
            const auto polished = renderVocalLike(48000.0, 0.5f);
            const auto rmsDeltaDb = juce::Decibels::gainToDecibels(
                polished.outputRms / polished.inputRms);

            expect(polished.outputCrest < dry.outputCrest - 0.03f,
                   "Polish should reduce crest factor measurably (dry "
                       + juce::String(dry.outputCrest, 4) + ", polished "
                       + juce::String(polished.outputCrest, 4) + ")");
            expectWithinAbsoluteError(rmsDeltaDb, 0.0f, 0.75f);
        }

        beginTest("maximum amount is finite and bounded before output safety");
        {
            VocalPolish polish;
            polish.prepare({96000.0, testBlockSize, 2});
            polish.setTargetSettings({1.0f});
            juce::AudioBuffer<float> buffer(2, testBlockSize);

            for (int iteration = 0; iteration < 1000; ++iteration)
            {
                for (int channel = 0; channel < buffer.getNumChannels();
                     ++channel)
                    for (int sample = 0; sample < buffer.getNumSamples();
                         ++sample)
                        buffer.setSample(channel, sample,
                                         ((sample + iteration) & 1) == 0
                                             ? 1.0f
                                             : -1.0f);

                polish.process(buffer);

                for (int channel = 0; channel < buffer.getNumChannels();
                     ++channel)
                    for (int sample = 0; sample < buffer.getNumSamples();
                         ++sample)
                    {
                        const auto value = buffer.getSample(channel, sample);
                        expect(std::isfinite(value));
                        expectLessOrEqual(std::abs(value), 1.2f);
                    }
            }
        }

        beginTest("presence response is consistent across supported sample rates");
        {
            const auto gain44100 = renderSineGainDb(44100.0, 3400.0);
            const auto gain48000 = renderSineGainDb(48000.0, 3400.0);
            const auto gain88200 = renderSineGainDb(88200.0, 3400.0);
            const auto gain96000 = renderSineGainDb(96000.0, 3400.0);
            const auto minimum =
                juce::jmin(gain44100, gain48000, gain88200, gain96000);
            const auto maximum =
                juce::jmax(gain44100, gain48000, gain88200, gain96000);

            expect(maximum - minimum < 1.0f);
            expect(gain48000 > 0.05f,
                   "50 percent Polish should add measurable presence");
        }
    }
};

VocalPolishTests vocalPolishTests;
}
