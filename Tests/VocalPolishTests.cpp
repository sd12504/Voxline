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

struct WindowMetrics
{
    float rms {};
    float brightness {};
};

WindowMetrics measureWindow(const juce::AudioBuffer<float>& buffer,
                            int startSample,
                            int sampleCount) noexcept
{
    double squares = 0.0;
    double differenceSquares = 0.0;
    auto previous = buffer.getSample(0, juce::jmax(0, startSample - 1));

    for (int sample = startSample; sample < startSample + sampleCount;
         ++sample)
    {
        const auto value = buffer.getSample(0, sample);
        const auto difference = value - previous;
        squares += static_cast<double>(value) * value;
        differenceSquares += static_cast<double>(difference) * difference;
        previous = value;
    }

    const auto rms =
        static_cast<float>(std::sqrt(squares / sampleCount));
    const auto differenceRms =
        static_cast<float>(std::sqrt(differenceSquares / sampleCount));
    return {rms, differenceRms / juce::jmax(1.0e-9f, rms)};
}

float absoluteDeltaDb(float first, float second) noexcept
{
    return std::abs(juce::Decibels::gainToDecibels(
        juce::jmax(1.0e-9f, first) / juce::jmax(1.0e-9f, second)));
}

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

        beginTest("Polish does not hide a hard output limiter");
        {
            VocalPolish polish;
            polish.prepare({48000.0, testBlockSize, 1});
            polish.setTargetSettings({0.01f});
            juce::AudioBuffer<float> buffer(1, testBlockSize);
            float maximum = 0.0f;

            for (int iteration = 0; iteration < 500; ++iteration)
            {
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample(0, sample, 1.5f);

                polish.process(buffer);

                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    const auto value = buffer.getSample(0, sample);
                    expect(std::isfinite(value));
                    maximum = juce::jmax(maximum, value);
                }
            }

            expect(maximum > 1.2f,
                   "A low Amount must preserve an over-range input for the "
                   "downstream Emergency Soft Clip");
            expect(maximum < 1.6f);
        }

        beginTest("zero stays dry while warm state survives 50 to 0 to 50");
        {
            constexpr double sampleRate = 48000.0;
            constexpr int warmupSamples = 48000;
            constexpr int zeroSamples = 48000;
            constexpr int restartSamples = 24000;
            VocalPolish toggled;
            VocalPolish reference;
            toggled.prepare({sampleRate, testBlockSize, 1});
            reference.prepare({sampleRate, testBlockSize, 1});
            toggled.setTargetSettings({0.5f});
            reference.setTargetSettings({0.5f});
            juce::AudioBuffer<float> toggledBlock(1, testBlockSize);
            juce::AudioBuffer<float> referenceBlock(1, testBlockSize);
            juce::AudioBuffer<float> dryBlock(1, testBlockSize);
            juce::AudioBuffer<float> toggledCapture(1, restartSamples);
            juce::AudioBuffer<float> referenceCapture(1, restartSamples);
            int64_t timeline = 0;

            const auto processStage =
                [&](int stageSamples, float inputScale, bool capture,
                    bool requireExactDry)
            {
                auto captured = 0;
                for (int first = 0; first < stageSamples;
                     first += testBlockSize)
                {
                    const auto count =
                        juce::jmin(testBlockSize, stageSamples - first);
                    toggledBlock.setSize(1, count, false, false, true);
                    fillVocalLikeBlock(toggledBlock, sampleRate, timeline);
                    toggledBlock.applyGain(inputScale);
                    dryBlock.makeCopyOf(toggledBlock, true);
                    referenceBlock.makeCopyOf(toggledBlock, true);
                    toggled.process(toggledBlock);
                    reference.process(referenceBlock);

                    if (requireExactDry
                        && first >= stageSamples - testBlockSize * 2)
                        for (int sample = 0; sample < count; ++sample)
                            expectEquals(toggledBlock.getSample(0, sample),
                                         dryBlock.getSample(0, sample));

                    if (capture)
                    {
                        toggledCapture.copyFrom(0, captured, toggledBlock, 0,
                                                0, count);
                        referenceCapture.copyFrom(0, captured, referenceBlock,
                                                  0, 0, count);
                        captured += count;
                    }

                    timeline += count;
                }
            };

            processStage(warmupSamples, 0.25f, false, false);
            toggled.setTargetSettings({0.0f});
            processStage(zeroSamples, 1.0f, false, true);
            toggled.setTargetSettings({0.5f});
            processStage(restartSamples, 1.0f, true, false);

            const std::array<int, 4> windowEnds {480, 2400, 4800, 24000};
            const std::array<float, 4> rmsLimitsDb {2.0f, 1.0f, 0.75f, 0.5f};
            const std::array<float, 4> colourLimitsDb {
                1.5f, 1.0f, 0.75f, 0.5f};

            for (size_t index = 0; index < windowEnds.size(); ++index)
            {
                const auto end = windowEnds[index];
                const auto length = juce::jmin(480, end);
                const auto toggledMetrics =
                    measureWindow(toggledCapture, end - length, length);
                const auto referenceMetrics =
                    measureWindow(referenceCapture, end - length, length);
                expect(absoluteDeltaDb(toggledMetrics.rms,
                                       referenceMetrics.rms)
                       < rmsLimitsDb[index]);
                expect(absoluteDeltaDb(toggledMetrics.brightness,
                                       referenceMetrics.brightness)
                       < colourLimitsDb[index]);
            }
        }

        beginTest("silence restart keeps early level and colour continuous");
        {
            constexpr double sampleRate = 48000.0;
            VocalPolish polish;
            polish.prepare({sampleRate, testBlockSize, 1});
            polish.setTargetSettings({0.5f});
            juce::AudioBuffer<float> block(1, testBlockSize);
            int64_t timeline = 0;

            for (int first = 0; first < 48000; first += testBlockSize)
            {
                fillVocalLikeBlock(block, sampleRate, timeline);
                polish.process(block);
                timeline += testBlockSize;
            }

            block.clear();
            for (int first = 0; first < 48000; first += testBlockSize)
                polish.process(block);

            juce::AudioBuffer<float> capture(1, 48000);
            for (int first = 0; first < capture.getNumSamples();
                 first += testBlockSize)
            {
                const auto count =
                    juce::jmin(testBlockSize,
                               capture.getNumSamples() - first);
                block.setSize(1, count, false, false, true);
                fillVocalLikeBlock(block, sampleRate, timeline);
                polish.process(block);
                capture.copyFrom(0, first, block, 0, 0, count);
                timeline += count;
            }

            const auto steady = measureWindow(capture, 43200, 4800);
            const std::array<int, 4> windowEnds {480, 2400, 4800, 24000};
            const std::array<float, 4> rmsLimitsDb {2.0f, 1.0f, 0.75f, 0.5f};
            const std::array<float, 4> colourLimitsDb {
                1.5f, 1.0f, 0.75f, 0.5f};

            for (size_t index = 0; index < windowEnds.size(); ++index)
            {
                const auto end = windowEnds[index];
                const auto length = juce::jmin(480, end);
                const auto early =
                    measureWindow(capture, end - length, length);
                expect(absoluteDeltaDb(early.rms, steady.rms)
                       < rmsLimitsDb[index]);
                expect(absoluteDeltaDb(early.brightness, steady.brightness)
                       < colourLimitsDb[index]);
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
