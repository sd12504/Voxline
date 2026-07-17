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
        juce::AudioBuffer<float>(2, testBlockSize)
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
