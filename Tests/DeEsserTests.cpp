#include <JuceHeader.h>

#include "../Source/DSP/DeEsser.h"

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr int testBlockSize = 512;

void fillTwoTone(juce::AudioBuffer<float>& buffer,
                 double sampleRate,
                 int64_t startSample,
                 bool sibilanceOnRight = true)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto time = static_cast<float>(
            static_cast<double>(startSample + sample) / sampleRate);
        const auto low = 0.20f * std::sin(
            juce::MathConstants<float>::twoPi * 1000.0f * time);
        const auto sibilance = 0.35f * std::sin(
            juce::MathConstants<float>::twoPi * 8000.0f * time);

        buffer.setSample(0, sample, low + sibilance);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, sample, low + (sibilanceOnRight ? sibilance : 0.0f));
    }
}

float componentAmplitude(const juce::AudioBuffer<float>& buffer,
                         int channel,
                         double sampleRate,
                         int64_t startSample,
                         float frequency)
{
    auto sineProjection = 0.0;
    auto cosineProjection = 0.0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
            * static_cast<double>(frequency)
            * static_cast<double>(startSample + sample) / sampleRate;
        const auto value = static_cast<double>(buffer.getSample(channel, sample));
        sineProjection += value * std::sin(phase);
        cosineProjection += value * std::cos(phase);
    }

    const auto scale = 2.0 / static_cast<double>(buffer.getNumSamples());
    return static_cast<float>(
        scale * std::sqrt(sineProjection * sineProjection
                          + cosineProjection * cosineProjection));
}

float gainChangeDb(float before, float after)
{
    return juce::Decibels::gainToDecibels(
        after / juce::jmax(before, 1.0e-9f), -120.0f);
}

float bufferRms(const juce::AudioBuffer<float>& buffer, int channel)
{
    return buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
}

void fillTone(juce::AudioBuffer<float>& buffer,
              double sampleRate,
              int64_t startSample,
              float frequency,
              float amplitude = 0.35f)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi
            * static_cast<double>(frequency)
            * static_cast<double>(startSample + sample) / sampleRate;
        const auto value = amplitude * static_cast<float>(std::sin(phase));
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

struct RenderResult
{
    juce::AudioBuffer<float> dry;
    juce::AudioBuffer<float> wet;
    int64_t startSample {};
    Voxline::Dsp::DeEssMetrics metrics;
};

RenderResult renderToSteadyState(Voxline::Dsp::DeEsserSettings settings,
                                 bool sibilanceOnRight = true)
{
    Voxline::Dsp::DeEsser deEsser;
    deEsser.prepare({testSampleRate, testBlockSize, 2});
    deEsser.setTargetSettings(settings);

    RenderResult result {
        juce::AudioBuffer<float>(2, testBlockSize),
        juce::AudioBuffer<float>(2, testBlockSize),
        0,
        {}
    };

    int64_t samplePosition = 0;
    for (int block = 0; block < 120; ++block)
    {
        fillTwoTone(result.dry, testSampleRate, samplePosition, sibilanceOnRight);
        result.wet.makeCopyOf(result.dry);
        result.metrics = deEsser.process(result.wet);
        result.startSample = samplePosition;
        samplePosition += testBlockSize;
    }

    return result;
}

juce::AudioBuffer<float> renderDetectorMonitor(float frequency,
                                               Voxline::Dsp::DeEssMode mode)
{
    Voxline::Dsp::DeEsser deEsser;
    deEsser.prepare({testSampleRate, testBlockSize, 2});
    deEsser.setTargetSettings({
        1.0f, 6500.0f, -18.0f, 6.0f, mode
    });

    juce::AudioBuffer<float> buffer(2, testBlockSize);
    int64_t position = 0;
    for (int block = 0; block < 80; ++block)
    {
        fillTone(buffer, testSampleRate, position, frequency);
        deEsser.process(buffer, Voxline::Dsp::DeEssMonitor::detector);
        position += testBlockSize;
    }
    return buffer;
}

struct SegmentedRender
{
    std::vector<float> left;
    Voxline::Dsp::DeEssMetrics metrics;
};

SegmentedRender renderWithSegmentation(int requestedBlockSize)
{
    constexpr int totalSamples = 18000;
    constexpr std::array<int, 3> eventSamples {4000, 8000, 12000};

    Voxline::Dsp::DeEsser deEsser;
    deEsser.prepare({testSampleRate, 2048, 2});
    auto settings = Voxline::Dsp::DeEsserSettings {
        0.75f, 6500.0f, -24.0f, 9.0f, Voxline::Dsp::DeEssMode::split
    };
    deEsser.setTargetSettings(settings);

    SegmentedRender result;
    result.left.resize(totalSamples);

    auto position = 0;
    while (position < totalSamples)
    {
        if (position == eventSamples[0])
        {
            settings.mode = Voxline::Dsp::DeEssMode::wide;
            deEsser.setTargetSettings(settings);
        }
        else if (position == eventSamples[1])
        {
            settings.focusHz = 9000.0f;
            deEsser.setTargetSettings(settings);
        }
        else if (position == eventSamples[2])
        {
            settings.amount = 0.0f;
            deEsser.setTargetSettings(settings);
        }

        auto samplesThisBlock = juce::jmin(requestedBlockSize,
                                           totalSamples - position);
        for (const auto event : eventSamples)
            if (event > position)
                samplesThisBlock = juce::jmin(samplesThisBlock,
                                              event - position);

        juce::AudioBuffer<float> block(2, samplesThisBlock);
        fillTwoTone(block, testSampleRate, position);
        result.metrics = deEsser.process(block);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
            result.left[static_cast<size_t>(position + sample)] =
                block.getSample(0, sample);
        position += samplesThisBlock;
    }

    return result;
}

float measureAttackMs(double sampleRate, int preparedBlockSize)
{
    Voxline::Dsp::DeEsser deEsser;
    deEsser.prepare({sampleRate, preparedBlockSize, 2});
    deEsser.setTargetSettings({
        1.0f, 5000.0f, -40.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
    });

    juce::AudioBuffer<float> sampleBuffer(2, 1);
    const auto maximumSamples = static_cast<int>(sampleRate * 0.100);

    for (int sample = 0; sample < maximumSamples; ++sample)
    {
        const auto value = 0.5f * std::sin(
            juce::MathConstants<float>::twoPi * 8000.0f
            * static_cast<float>(sample) / static_cast<float>(sampleRate));
        sampleBuffer.setSample(0, 0, value);
        sampleBuffer.setSample(1, 0, value);

        if (deEsser.process(sampleBuffer).reductionDb >= 3.0f)
            return static_cast<float>(sample * 1000.0 / sampleRate);
    }

    return std::numeric_limits<float>::infinity();
}

float measureReleaseMs(double sampleRate, int preparedBlockSize)
{
    Voxline::Dsp::DeEsser deEsser;
    deEsser.prepare({sampleRate, preparedBlockSize, 2});
    deEsser.setTargetSettings({
        1.0f, 5000.0f, -40.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
    });

    juce::AudioBuffer<float> sampleBuffer(2, 1);
    const auto warmupSamples = static_cast<int>(sampleRate * 0.250);
    for (int sample = 0; sample < warmupSamples; ++sample)
    {
        const auto value = 0.5f * std::sin(
            juce::MathConstants<float>::twoPi * 8000.0f
            * static_cast<float>(sample) / static_cast<float>(sampleRate));
        sampleBuffer.setSample(0, 0, value);
        sampleBuffer.setSample(1, 0, value);
        deEsser.process(sampleBuffer);
    }

    const auto startingReduction = deEsser.process(sampleBuffer).reductionDb;
    const auto threshold = startingReduction / std::exp(1.0f);
    const auto maximumSamples = static_cast<int>(sampleRate * 0.500);

    deEsser.setTargetSettings({
        1.0f, 5000.0f, 0.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
    });
    sampleBuffer.clear();
    for (int sample = 0; sample < maximumSamples; ++sample)
    {
        if (deEsser.process(sampleBuffer).reductionDb <= threshold)
            return static_cast<float>(sample * 1000.0 / sampleRate);
    }

    return std::numeric_limits<float>::infinity();
}

class DeEsserTests final : public juce::UnitTest
{
public:
    DeEsserTests()
        : juce::UnitTest("De-esser", "VOXLINE") {}

    void runTest() override
    {
        beginTest("amount zero is sample-exact null");
        {
            Voxline::Dsp::DeEsser deEsser;
            deEsser.prepare({testSampleRate, testBlockSize, 2});
            deEsser.setTargetSettings({});

            juce::AudioBuffer<float> buffer(2, testBlockSize);
            fillTwoTone(buffer, testSampleRate, 0);
            juce::AudioBuffer<float> expected;
            expected.makeCopyOf(buffer);

            const auto metrics = deEsser.process(buffer);

            expect(buffer == expected);
            expectWithinAbsoluteError(metrics.reductionDb, 0.0f, 1.0e-7f);
            expect(! metrics.sActive);
        }

        beginTest("split mode attenuates sibilance without changing the low component");
        {
            const auto result = renderToSteadyState({
                1.0f, 5000.0f, -40.0f, 12.0f, Voxline::Dsp::DeEssMode::split
            });
            const auto lowChange = gainChangeDb(
                componentAmplitude(result.dry, 0, testSampleRate,
                                   result.startSample, 1000.0f),
                componentAmplitude(result.wet, 0, testSampleRate,
                                   result.startSample, 1000.0f));
            const auto highChange = gainChangeDb(
                componentAmplitude(result.dry, 0, testSampleRate,
                                   result.startSample, 8000.0f),
                componentAmplitude(result.wet, 0, testSampleRate,
                                   result.startSample, 8000.0f));

            expect(highChange <= -3.0f);
            expect(std::abs(lowChange) < 0.75f);
        }

        beginTest("wide mode attenuates both components when sibilance triggers");
        {
            const auto result = renderToSteadyState({
                1.0f, 5000.0f, -40.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
            });
            const auto lowChange = gainChangeDb(
                componentAmplitude(result.dry, 0, testSampleRate,
                                   result.startSample, 1000.0f),
                componentAmplitude(result.wet, 0, testSampleRate,
                                   result.startSample, 1000.0f));
            const auto highChange = gainChangeDb(
                componentAmplitude(result.dry, 0, testSampleRate,
                                   result.startSample, 8000.0f),
                componentAmplitude(result.wet, 0, testSampleRate,
                                   result.startSample, 8000.0f));

            expect(lowChange <= -3.0f);
            expect(highChange <= -3.0f);
        }

        beginTest("left-only sibilance applies linked reduction to both channels");
        {
            const auto result = renderToSteadyState({
                1.0f, 5000.0f, -40.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
            }, false);
            const auto leftLowChange = gainChangeDb(
                componentAmplitude(result.dry, 0, testSampleRate,
                                   result.startSample, 1000.0f),
                componentAmplitude(result.wet, 0, testSampleRate,
                                   result.startSample, 1000.0f));
            const auto rightLowChange = gainChangeDb(
                componentAmplitude(result.dry, 1, testSampleRate,
                                   result.startSample, 1000.0f),
                componentAmplitude(result.wet, 1, testSampleRate,
                                   result.startSample, 1000.0f));

            expect(leftLowChange <= -3.0f);
            expect(rightLowChange <= -3.0f);
            expectWithinAbsoluteError(leftLowChange, rightLowChange, 0.15f);
        }

        beginTest("reduction obeys the user maximum range");
        {
            const auto naturalRange = renderToSteadyState({
                1.0f, 5000.0f, -60.0f, 6.0f, Voxline::Dsp::DeEssMode::wide
            });
            const auto extendedRange = renderToSteadyState({
                1.0f, 5000.0f, -60.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
            });

            expect(naturalRange.metrics.reductionDb <= 6.001f);
            expect(extendedRange.metrics.reductionDb <= 12.001f);
            expect(extendedRange.metrics.reductionDb > 9.0f);
        }

        beginTest("amount controls reduction without double scaling");
        {
            const auto result = renderToSteadyState({
                0.5f, 6500.0f, -24.0f, 12.0f, Voxline::Dsp::DeEssMode::wide
            });
            expect(result.metrics.reductionDb > 2.0f);
            expect(result.metrics.reductionDb <= 6.001f);
        }

        beginTest("amount transition releases smoothly then becomes exact null");
        {
            Voxline::Dsp::DeEsser deEsser;
            deEsser.prepare({testSampleRate, 1, 2});
            auto settings = Voxline::Dsp::DeEsserSettings {
                1.0f, 6500.0f, -40.0f, 12.0f,
                Voxline::Dsp::DeEssMode::wide
            };
            deEsser.setTargetSettings(settings);

            juce::AudioBuffer<float> sampleBuffer(2, 1);
            for (int sample = 0; sample < 12000; ++sample)
            {
                const auto value = 0.5f * std::sin(
                    juce::MathConstants<float>::twoPi * 8000.0f
                    * static_cast<float>(sample) / 48000.0f);
                sampleBuffer.setSample(0, 0, value);
                sampleBuffer.setSample(1, 0, value);
                deEsser.process(sampleBuffer);
            }

            settings.amount = 0.0f;
            deEsser.setTargetSettings(settings);
            sampleBuffer.setSample(0, 0, 0.5f);
            sampleBuffer.setSample(1, 0, 0.5f);
            const auto firstReleaseMetrics = deEsser.process(sampleBuffer);
            expect(firstReleaseMetrics.reductionDb > 3.0f);
            expect(sampleBuffer.getSample(0, 0) < 0.4f);

            for (int sample = 0; sample < 48000; ++sample)
            {
                sampleBuffer.setSample(0, 0, 0.0f);
                sampleBuffer.setSample(1, 0, 0.0f);
                deEsser.process(sampleBuffer);
            }

            juce::AudioBuffer<float> exactNull(2, 257);
            fillTwoTone(exactNull, testSampleRate, 12000);
            juce::AudioBuffer<float> expected;
            expected.makeCopyOf(exactNull);
            deEsser.process(exactNull);
            expect(exactNull == expected);
        }

        beginTest("mode and focus automation start without a sample jump");
        {
            Voxline::Dsp::DeEsser reference;
            Voxline::Dsp::DeEsser automated;
            reference.prepare({testSampleRate, 1, 2});
            automated.prepare({testSampleRate, 1, 2});
            auto settings = Voxline::Dsp::DeEsserSettings {
                1.0f, 5000.0f, -40.0f, 12.0f,
                Voxline::Dsp::DeEssMode::split
            };
            reference.setTargetSettings(settings);
            automated.setTargetSettings(settings);

            juce::AudioBuffer<float> referenceSample(2, 1);
            juce::AudioBuffer<float> automatedSample(2, 1);
            for (int sample = 0; sample < 12000; ++sample)
            {
                const auto time = static_cast<float>(sample) / 48000.0f;
                const auto value =
                    0.20f * std::sin(juce::MathConstants<float>::twoPi
                                     * 1000.0f * time)
                    + 0.35f * std::sin(juce::MathConstants<float>::twoPi
                                      * 8000.0f * time);
                referenceSample.setSample(0, 0, value);
                referenceSample.setSample(1, 0, value);
                automatedSample.makeCopyOf(referenceSample);
                reference.process(referenceSample);
                automated.process(automatedSample);
            }

            settings.mode = Voxline::Dsp::DeEssMode::wide;
            settings.focusHz = 9000.0f;
            automated.setTargetSettings(settings);
            referenceSample.setSample(0, 0, 0.5f);
            referenceSample.setSample(1, 0, 0.5f);
            automatedSample.makeCopyOf(referenceSample);
            reference.process(referenceSample);
            automated.process(automatedSample);
            expectWithinAbsoluteError(automatedSample.getSample(0, 0),
                                      referenceSample.getSample(0, 0),
                                      1.0e-3f);
        }

        beginTest("focus detector rejects energy far below its centre");
        {
            const auto centred = renderDetectorMonitor(
                6500.0f, Voxline::Dsp::DeEssMode::split);
            const auto low = renderDetectorMonitor(
                1000.0f, Voxline::Dsp::DeEssMode::split);
            expect(bufferRms(centred, 0) > bufferRms(low, 0) * 5.0f);
        }

        beginTest("Listen S returns detector sidechain in wide mode");
        {
            Voxline::Dsp::DeEsser deEsser;
            deEsser.prepare({testSampleRate, testBlockSize, 2});
            deEsser.setTargetSettings({
                1.0f, 6500.0f, -40.0f, 12.0f,
                Voxline::Dsp::DeEssMode::wide
            });

            juce::AudioBuffer<float> monitored(2, testBlockSize);
            int64_t position = 0;
            for (int block = 0; block < 80; ++block)
            {
                fillTwoTone(monitored, testSampleRate, position);
                deEsser.process(monitored,
                                Voxline::Dsp::DeEssMonitor::detector);
                position += testBlockSize;
            }

            const auto low = componentAmplitude(
                monitored, 0, testSampleRate,
                position - testBlockSize, 1000.0f);
            const auto high = componentAmplitude(
                monitored, 0, testSampleRate,
                position - testBlockSize, 8000.0f);
            expect(high > low * 5.0f);
        }

        beginTest("processing is independent of runtime block segmentation");
        {
            const auto reference = renderWithSegmentation(1);
            for (const auto blockSize : {7, 64, 511, 2048})
            {
                const auto candidate = renderWithSegmentation(blockSize);
                auto maximumDifference = 0.0f;
                for (size_t sample = 0; sample < reference.left.size(); ++sample)
                    maximumDifference = juce::jmax(
                        maximumDifference,
                        std::abs(reference.left[sample]
                                 - candidate.left[sample]));
                expect(maximumDifference < 1.0e-6f,
                       "block " + juce::String(blockSize)
                           + ", max diff "
                           + juce::String(maximumDifference, 9));
                expectWithinAbsoluteError(candidate.metrics.reductionDb,
                                          reference.metrics.reductionDb,
                                          1.0e-6f);
            }
        }

        beginTest("attack and release use elapsed sample time");
        {
            const auto attack441 = measureAttackMs(44100.0, 32);
            const auto attack96 = measureAttackMs(96000.0, 512);
            const auto release441 = measureReleaseMs(44100.0, 32);
            const auto release96 = measureReleaseMs(96000.0, 512);
            const auto timingValues =
                "attack 44.1/96 kHz: " + juce::String(attack441)
                + " / " + juce::String(attack96)
                + " ms, release 44.1/96 kHz: "
                + juce::String(release441) + " / "
                + juce::String(release96) + " ms";

            expect(std::isfinite(attack441), timingValues);
            expect(std::isfinite(attack96), timingValues);
            expect(std::isfinite(release441), timingValues);
            expect(std::isfinite(release96), timingValues);
            expect(std::abs(attack441 - attack96) < 2.0f, timingValues);
            expect(std::abs(release441 - release96) < 2.0f, timingValues);
        }
    }
};

DeEsserTests deEsserTests;
}
