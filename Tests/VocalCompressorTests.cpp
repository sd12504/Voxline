#include <JuceHeader.h>

#include "../Source/DSP/VocalCompressor.h"

namespace
{
constexpr int testBlockSize = 512;
constexpr double testSampleRate = 48000.0;
constexpr float referenceRms =
    0.2511886432f; // -12 dBFS

void fillVocalLike(juce::AudioBuffer<float>& buffer,
                   double sampleRate,
                   int64_t startSample,
                   float leftScale = 1.0f,
                   float rightScale = 1.0f)
{
    constexpr float normalisation = 0.328824f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto time = static_cast<double>(startSample + sample) / sampleRate;
        const auto voice = normalisation * (
            std::sin(juce::MathConstants<double>::twoPi * 180.0 * time)
            + 0.35 * std::sin(
                juce::MathConstants<double>::twoPi * 540.0 * time)
            + 0.20 * std::sin(
                juce::MathConstants<double>::twoPi * 1440.0 * time));

        buffer.setSample(0, sample,
                         static_cast<float>(voice) * leftScale);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, sample,
                             static_cast<float>(voice) * rightScale);
    }
}

bool buffersAreExactlyEqual(const juce::AudioBuffer<float>& left,
                            const juce::AudioBuffer<float>& right)
{
    if (left.getNumChannels() != right.getNumChannels()
        || left.getNumSamples() != right.getNumSamples())
        return false;

    for (int channel = 0; channel < left.getNumChannels(); ++channel)
        for (int sample = 0; sample < left.getNumSamples(); ++sample)
            if (left.getSample(channel, sample)
                != right.getSample(channel, sample))
                return false;

    return true;
}

Voxline::Dsp::CompressorMetrics renderToSteadyState(
    Voxline::Dsp::CompressorSettings settings,
    float leftScale = 1.0f,
    float rightScale = 1.0f)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, testBlockSize, 2});
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> buffer(2, testBlockSize);
    Voxline::Dsp::CompressorMetrics metrics;
    int64_t position = 0;

    for (int block = 0; block < 400; ++block)
    {
        fillVocalLike(buffer, testSampleRate, position,
                      leftScale, rightScale);
        metrics = compressor.process(buffer);
        position += testBlockSize;
    }

    return metrics;
}

float measureAttackMs(double sampleRate)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({sampleRate, 512, 2});
    compressor.setTargetSettings({
        100.0f, 0.0f, 3.0f, 20.0f, 120.0f,
        1.0f, 0.0f, false
    });

    juce::AudioBuffer<float> sampleBuffer(2, 1);
    sampleBuffer.setSample(0, 0, referenceRms);
    sampleBuffer.setSample(1, 0, referenceRms);
    constexpr float targetReduction = 9.0f;
    const auto crossing = targetReduction * (1.0f - std::exp(-1.0f));

    const auto maximumSamples = static_cast<int>(sampleRate * 0.25);
    for (int sample = 0; sample < maximumSamples; ++sample)
    {
        sampleBuffer.setSample(0, 0, referenceRms);
        sampleBuffer.setSample(1, 0, referenceRms);
        if (compressor.process(sampleBuffer).gainReductionDb >= crossing)
            return static_cast<float>(sample * 1000.0 / sampleRate);
    }

    return std::numeric_limits<float>::infinity();
}

float measureReleaseMs(double sampleRate)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({sampleRate, 512, 2});
    compressor.setTargetSettings({
        100.0f, 0.0f, 3.0f, 1.0f, 120.0f,
        1.0f, 0.0f, false
    });

    juce::AudioBuffer<float> sampleBuffer(2, 1);
    sampleBuffer.setSample(0, 0, referenceRms);
    sampleBuffer.setSample(1, 0, referenceRms);
    for (int sample = 0;
         sample < static_cast<int>(sampleRate * 0.5); ++sample)
    {
        sampleBuffer.setSample(0, 0, referenceRms);
        sampleBuffer.setSample(1, 0, referenceRms);
        compressor.process(sampleBuffer);
    }

    sampleBuffer.setSample(0, 0, referenceRms);
    sampleBuffer.setSample(1, 0, referenceRms);
    const auto startingReduction =
        compressor.process(sampleBuffer).gainReductionDb;
    compressor.setTargetSettings({
        0.0f, 0.0f, 3.0f, 1.0f, 120.0f,
        1.0f, 0.0f, false
    });
    sampleBuffer.clear();

    const auto crossing = startingReduction * std::exp(-1.0f);
    const auto maximumSamples = static_cast<int>(sampleRate * 0.5);
    for (int sample = 0; sample < maximumSamples; ++sample)
        if (compressor.process(sampleBuffer).gainReductionDb <= crossing)
            return static_cast<float>(sample * 1000.0 / sampleRate);

    return std::numeric_limits<float>::infinity();
}

struct GainPair
{
    float leftDb {};
    float rightDb {};
};

GainPair measureLinkedStereoGain()
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, testBlockSize, 2});
    compressor.setTargetSettings({
        75.0f, 0.0f, 3.0f, 5.0f, 80.0f,
        1.0f, 0.0f, false
    });

    juce::AudioBuffer<float> dry(2, testBlockSize);
    juce::AudioBuffer<float> wet(2, testBlockSize);
    int64_t position = 0;
    for (int block = 0; block < 300; ++block)
    {
        fillVocalLike(dry, testSampleRate, position, 1.0f, 0.25f);
        wet.makeCopyOf(dry);
        compressor.process(wet);
        position += testBlockSize;
    }

    return {
        juce::Decibels::gainToDecibels(
            wet.getRMSLevel(0, 0, testBlockSize)
                / dry.getRMSLevel(0, 0, testBlockSize)),
        juce::Decibels::gainToDecibels(
            wet.getRMSLevel(1, 0, testBlockSize)
                / dry.getRMSLevel(1, 0, testBlockSize))
    };
}

struct LevelResult
{
    float inputRmsDb {};
    float outputRmsDb {};
    float outputPeak {};
};

LevelResult measureAutoMakeup()
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, testBlockSize, 2});
    compressor.setTargetSettings({
        75.0f, 0.0f, 3.0f, 10.0f, 80.0f,
        1.0f, 0.0f, true
    });

    juce::AudioBuffer<float> dry(2, testBlockSize);
    juce::AudioBuffer<float> wet(2, testBlockSize);
    double drySquares = 0.0;
    double wetSquares = 0.0;
    float outputPeak = 0.0f;
    int64_t position = 0;

    for (int block = 0; block < 700; ++block)
    {
        fillVocalLike(dry, testSampleRate, position);
        wet.makeCopyOf(dry);
        compressor.process(wet);
        position += testBlockSize;

        if (block < 500)
            continue;

        for (int sample = 0; sample < testBlockSize; ++sample)
        {
            const auto drySample = dry.getSample(0, sample);
            const auto wetSample = wet.getSample(0, sample);
            drySquares += static_cast<double>(drySample * drySample);
            wetSquares += static_cast<double>(wetSample * wetSample);
            outputPeak = juce::jmax(outputPeak, std::abs(wetSample));
        }
    }

    constexpr auto measuredSamples = 200 * testBlockSize;
    return {
        juce::Decibels::gainToDecibels(
            static_cast<float>(std::sqrt(drySquares / measuredSamples))),
        juce::Decibels::gainToDecibels(
            static_cast<float>(std::sqrt(wetSquares / measuredSamples))),
        outputPeak
    };
}

class VocalCompressorTests final : public juce::UnitTest
{
public:
    VocalCompressorTests()
        : juce::UnitTest("Vocal compressor", "VOXLINE") {}

    void runTest() override
    {
        beginTest("amount zero is sample-exact null");
        {
            Voxline::Dsp::VocalCompressor compressor;
            compressor.prepare({testSampleRate, testBlockSize, 2});
            compressor.setTargetSettings({});

            juce::AudioBuffer<float> actual(2, testBlockSize);
            fillVocalLike(actual, testSampleRate, 0);
            juce::AudioBuffer<float> expected;
            expected.makeCopyOf(actual);

            const auto metrics = compressor.process(actual);
            expect(buffersAreExactlyEqual(actual, expected));
            expectEquals(metrics.gainReductionDb, 0.0f);
        }

        beginTest("mix zero is sample-exact null");
        {
            Voxline::Dsp::VocalCompressor compressor;
            compressor.prepare({testSampleRate, testBlockSize, 2});
            compressor.setTargetSettings({
                100.0f, 0.0f, 6.0f, 1.0f, 80.0f,
                0.0f, 8.0f, true
            });

            juce::AudioBuffer<float> actual(2, testBlockSize);
            fillVocalLike(actual, testSampleRate, 0);
            juce::AudioBuffer<float> expected;
            expected.makeCopyOf(actual);

            compressor.process(actual);
            expect(buffersAreExactlyEqual(actual, expected));
        }

        beginTest("amount maps to calibrated steady target reduction");
        {
            struct Target
            {
                float amount;
                float minimumDb;
                float maximumDb;
            };
            constexpr Target targets[] {
                {25.0f, 1.0f, 2.0f},
                {50.0f, 3.0f, 4.0f},
                {75.0f, 5.0f, 7.0f},
                {100.0f, 8.0f, 10.0f}
            };

            for (const auto& target : targets)
            {
                const auto metrics = renderToSteadyState({
                    target.amount, 0.0f, 3.0f, 5.0f, 80.0f,
                    1.0f, 0.0f, false
                });
                expect(metrics.gainReductionDb >= target.minimumDb);
                expect(metrics.gainReductionDb <= target.maximumDb);
            }
        }

        beginTest("attack timing is sample-rate invariant");
        {
            const auto at48k = measureAttackMs(48000.0);
            const auto at96k = measureAttackMs(96000.0);
            expect(std::isfinite(at48k));
            expect(std::isfinite(at96k));
            expectWithinAbsoluteError(at48k, at96k, 2.0f);
        }

        beginTest("release timing is sample-rate invariant");
        {
            const auto at48k = measureReleaseMs(48000.0);
            const auto at96k = measureReleaseMs(96000.0);
            expect(std::isfinite(at48k));
            expect(std::isfinite(at96k));
            expectWithinAbsoluteError(at48k, at96k, 2.0f);
        }

        beginTest("linked detector applies the same stereo gain");
        {
            const auto gains = measureLinkedStereoGain();
            expectWithinAbsoluteError(gains.leftDb, gains.rightDb, 0.05f);
        }

        beginTest("auto makeup restores long-term level without clipping");
        {
            const auto levels = measureAutoMakeup();
            expectWithinAbsoluteError(
                levels.outputRmsDb, levels.inputRmsDb, 0.5f);
            expect(levels.outputPeak < 1.0f);
        }

        beginTest("reset clears the applied gain envelope");
        {
            Voxline::Dsp::VocalCompressor compressor;
            compressor.prepare({testSampleRate, testBlockSize, 2});
            compressor.setTargetSettings({
                100.0f, 0.0f, 3.0f, 1.0f, 80.0f,
                1.0f, 0.0f, false
            });

            juce::AudioBuffer<float> buffer(2, testBlockSize);
            fillVocalLike(buffer, testSampleRate, 0);
            for (int block = 0; block < 100; ++block)
                compressor.process(buffer);

            compressor.reset();
            buffer.clear();
            expectEquals(
                compressor.process(buffer).gainReductionDb, 0.0f);
        }
    }
};

VocalCompressorTests vocalCompressorTests;
}
