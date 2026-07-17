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

float renderOutputRms(Voxline::Dsp::CompressorSettings settings,
                      float inputScale = 1.0f)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, testBlockSize, 2});
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> buffer(2, testBlockSize);
    int64_t position = 0;
    for (int block = 0; block < 400; ++block)
    {
        fillVocalLike(buffer, testSampleRate, position,
                      inputScale, inputScale);
        compressor.process(buffer);
        position += testBlockSize;
    }

    return buffer.getRMSLevel(0, 0, buffer.getNumSamples());
}

struct OffTransitionResult
{
    float firstSample {};
    float previousSample {};
    bool exactDryAfterTransition {true};
    float finalReductionDb {};
};

OffTransitionResult renderOffTransition(bool turnAmountOff)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, 2048, 2});
    auto settings = Voxline::Dsp::CompressorSettings {
        75.0f, 25.0f, 4.0f, 5.0f, 80.0f,
        1.0f, 6.0f, true
    };
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> warmup(2, testBlockSize);
    warmup.clear();
    for (int sample = 0; sample < warmup.getNumSamples(); ++sample)
        for (int channel = 0; channel < warmup.getNumChannels(); ++channel)
            warmup.setSample(channel, sample, referenceRms);

    for (int block = 0; block < 400; ++block)
    {
        for (int sample = 0; sample < warmup.getNumSamples(); ++sample)
            for (int channel = 0;
                 channel < warmup.getNumChannels(); ++channel)
                warmup.setSample(channel, sample, referenceRms);
        compressor.process(warmup);
    }

    const auto previous = warmup.getSample(
        0, warmup.getNumSamples() - 1);
    if (turnAmountOff)
        settings.amount = 0.0f;
    else
        settings.mix = 0.0f;
    compressor.setTargetSettings(settings);

    // The wet gate has a fixed, sample-counted 20 ms transition.
    constexpr int transitionSamples =
        static_cast<int>(testSampleRate * 0.020);
    juce::AudioBuffer<float> transition(2, transitionSamples + 128);
    for (int sample = 0; sample < transition.getNumSamples(); ++sample)
        for (int channel = 0;
             channel < transition.getNumChannels(); ++channel)
            transition.setSample(channel, sample, referenceRms);

    const auto metrics = compressor.process(transition);
    auto exactDry = true;
    for (int sample = transitionSamples;
         sample < transition.getNumSamples(); ++sample)
        for (int channel = 0;
             channel < transition.getNumChannels(); ++channel)
            exactDry = exactDry
                && transition.getSample(channel, sample) == referenceRms;

    return {
        transition.getSample(0, 0),
        previous,
        exactDry,
        metrics.gainReductionDb
    };
}

std::vector<float> renderWithSegmentation(int requestedBlockSize)
{
    constexpr int totalSamples = 14000;
    constexpr std::array<int, 4> eventSamples {
        3000, 6500, 9000, 11500
    };

    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, 2048, 2});
    auto settings = Voxline::Dsp::CompressorSettings {
        25.0f, 0.0f, 2.0f, 10.0f, 100.0f,
        1.0f, 0.0f, false
    };
    compressor.setTargetSettings(settings);

    std::vector<float> rendered(static_cast<size_t>(totalSamples));
    auto position = 0;
    while (position < totalSamples)
    {
        if (position == eventSamples[0])
        {
            settings.amount = 100.0f;
            settings.sensitivity = 75.0f;
            settings.ratio = 6.0f;
            compressor.setTargetSettings(settings);
        }
        else if (position == eventSamples[1])
        {
            settings.mix = 0.2f;
            settings.makeupDb = 4.0f;
            compressor.setTargetSettings(settings);
        }
        else if (position == eventSamples[2])
        {
            settings.mix = 0.0f;
            compressor.setTargetSettings(settings);
        }
        else if (position == eventSamples[3])
        {
            settings.amount = 50.0f;
            settings.mix = 1.0f;
            compressor.setTargetSettings(settings);
        }

        auto samplesThisBlock = juce::jmin(
            requestedBlockSize, totalSamples - position);
        for (const auto event : eventSamples)
            if (event > position)
                samplesThisBlock = juce::jmin(
                    samplesThisBlock, event - position);

        juce::AudioBuffer<float> block(2, samplesThisBlock);
        fillVocalLike(block, testSampleRate, position);
        compressor.process(block);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
            rendered[static_cast<size_t>(position + sample)] =
                block.getSample(0, sample);
        position += samplesThisBlock;
    }

    return rendered;
}

struct RepeatedOffResult
{
    bool exactDryAfterTransition {true};
    float finalReductionDb {};
};

RepeatedOffResult renderRepeatedOffTarget(int requestedBlockSize,
                                          bool turnAmountOff)
{
    constexpr int transitionSamples =
        static_cast<int>(testSampleRate * 0.020);
    constexpr int totalSamples = transitionSamples * 3;

    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, 2048, 2});
    auto settings = Voxline::Dsp::CompressorSettings {
        75.0f, 25.0f, 4.0f, 5.0f, 80.0f,
        1.0f, 6.0f, true
    };
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> block(2, requestedBlockSize);
    for (int warmup = 0; warmup < 200; ++warmup)
    {
        block.setSize(2, requestedBlockSize, false, false, true);
        block.clear();
        for (int sample = 0; sample < block.getNumSamples(); ++sample)
            for (int channel = 0; channel < block.getNumChannels(); ++channel)
                block.setSample(channel, sample, referenceRms);
        compressor.process(block);
    }

    if (turnAmountOff)
        settings.amount = 0.0f;
    else
        settings.mix = 0.0f;

    auto exactDry = true;
    Voxline::Dsp::CompressorMetrics metrics;
    for (int position = 0; position < totalSamples;)
    {
        const auto samplesThisBlock =
            juce::jmin(requestedBlockSize, totalSamples - position);
        block.setSize(2, samplesThisBlock, false, false, true);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
            for (int channel = 0; channel < block.getNumChannels(); ++channel)
                block.setSample(channel, sample, referenceRms);

        // A processor may mirror APVTS values into DSP settings every block.
        compressor.setTargetSettings(settings);
        metrics = compressor.process(block);

        for (int sample = 0; sample < samplesThisBlock; ++sample)
            if (position + sample >= transitionSamples)
                for (int channel = 0;
                     channel < block.getNumChannels(); ++channel)
                    exactDry = exactDry
                        && block.getSample(channel, sample) == referenceRms;

        position += samplesThisBlock;
    }

    return {exactDry, metrics.gainReductionDb};
}

struct RetargetResult
{
    float boundaryDelta {};
    float finalOutput {};
    float expectedOutput {};
};

RetargetResult renderMidTransitionRetarget(int requestedBlockSize)
{
    constexpr int firstLegSamples = 240;
    constexpr int settleSamples =
        static_cast<int>(testSampleRate * 0.020) + 128;

    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({testSampleRate, 2048, 2});
    auto settings = Voxline::Dsp::CompressorSettings {
        75.0f, 0.0f, 4.0f, 1.0f, 2000.0f,
        1.0f, 0.0f, false
    };
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> block(2, requestedBlockSize);
    float fullyWet {};
    for (int warmup = 0; warmup < 400; ++warmup)
    {
        block.setSize(2, requestedBlockSize, false, false, true);
        for (int sample = 0; sample < block.getNumSamples(); ++sample)
            for (int channel = 0; channel < block.getNumChannels(); ++channel)
                block.setSample(channel, sample, referenceRms);
        compressor.process(block);
        fullyWet = block.getSample(0, block.getNumSamples() - 1);
    }

    settings.mix = 0.0f;
    compressor.setTargetSettings(settings);
    juce::AudioBuffer<float> firstLeg(2, firstLegSamples);
    for (int sample = 0; sample < firstLegSamples; ++sample)
        for (int channel = 0; channel < firstLeg.getNumChannels(); ++channel)
            firstLeg.setSample(channel, sample, referenceRms);
    compressor.process(firstLeg);
    const auto beforeRetarget =
        firstLeg.getSample(0, firstLeg.getNumSamples() - 1);

    settings.mix = 0.5f;
    float firstAfterRetarget {};
    float finalOutput {};
    for (int position = 0; position < settleSamples;)
    {
        const auto samplesThisBlock =
            juce::jmin(requestedBlockSize, settleSamples - position);
        block.setSize(2, samplesThisBlock, false, false, true);
        for (int sample = 0; sample < samplesThisBlock; ++sample)
            for (int channel = 0; channel < block.getNumChannels(); ++channel)
                block.setSample(channel, sample, referenceRms);

        compressor.setTargetSettings(settings);
        compressor.process(block);
        if (position == 0)
            firstAfterRetarget = block.getSample(0, 0);
        finalOutput = block.getSample(0, samplesThisBlock - 1);
        position += samplesThisBlock;
    }

    return {
        std::abs(firstAfterRetarget - beforeRetarget),
        finalOutput,
        referenceRms + 0.5f * (fullyWet - referenceRms)
    };
}

struct RatioReleaseResult
{
    float startingReductionDb {};
    float reductionAt100MsDb {};
    float finalReductionDb {};
    float maximumIncreaseDb {};
    float maximumSampleDelta {};
};

RatioReleaseResult renderRatioToUnity(double sampleRate)
{
    Voxline::Dsp::VocalCompressor compressor;
    compressor.prepare({sampleRate, 512, 2});
    auto settings = Voxline::Dsp::CompressorSettings {
        75.0f, 0.0f, 3.0f, 5.0f, 80.0f,
        1.0f, 0.0f, false
    };
    compressor.setTargetSettings(settings);

    juce::AudioBuffer<float> sampleBuffer(2, 1);
    Voxline::Dsp::CompressorMetrics metrics;
    for (int sample = 0; sample < static_cast<int>(sampleRate); ++sample)
    {
        sampleBuffer.setSample(0, 0, referenceRms);
        sampleBuffer.setSample(1, 0, referenceRms);
        metrics = compressor.process(sampleBuffer);
    }

    const auto startingReduction = metrics.gainReductionDb;
    auto previousReduction = startingReduction;
    auto previousOutput = sampleBuffer.getSample(0, 0);
    auto maximumIncrease = 0.0f;
    auto maximumSampleDelta = 0.0f;
    auto reductionAt100Ms = startingReduction;
    const auto totalSamples = static_cast<int>(sampleRate * 0.5);
    const auto sampleAt100Ms = static_cast<int>(sampleRate * 0.1);

    settings.ratio = 1.0f;
    compressor.setTargetSettings(settings);
    for (int sample = 0; sample < totalSamples; ++sample)
    {
        sampleBuffer.setSample(0, 0, referenceRms);
        sampleBuffer.setSample(1, 0, referenceRms);
        metrics = compressor.process(sampleBuffer);
        const auto output = sampleBuffer.getSample(0, 0);
        maximumIncrease = juce::jmax(
            maximumIncrease,
            metrics.gainReductionDb - previousReduction);
        maximumSampleDelta = juce::jmax(
            maximumSampleDelta, std::abs(output - previousOutput));
        previousReduction = metrics.gainReductionDb;
        previousOutput = output;
        if (sample == sampleAt100Ms)
            reductionAt100Ms = metrics.gainReductionDb;
    }

    return {
        startingReduction,
        reductionAt100Ms,
        metrics.gainReductionDb,
        maximumIncrease,
        maximumSampleDelta
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
            compressor.setTargetSettings({
                0.0f, 100.0f, 20.0f, 0.1f, 2000.0f,
                1.0f, 8.0f, true
            });

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

        beginTest("mix off crossfades then becomes exact dry");
        {
            const auto result = renderOffTransition(false);
            expect(std::abs(result.firstSample - result.previousSample)
                   < 0.02f);
            expect(result.exactDryAfterTransition);
        }

        beginTest("amount off crossfades then clears all wet state");
        {
            const auto result = renderOffTransition(true);
            expect(std::abs(result.firstSample - result.previousSample)
                   < 0.02f);
            expect(result.exactDryAfterTransition);
            expectEquals(result.finalReductionDb, 0.0f);
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
                const auto context = "Amount "
                    + juce::String(target.amount)
                    + " actual GR "
                    + juce::String(metrics.gainReductionDb);
                expect(metrics.gainReductionDb >= target.minimumDb,
                       context);
                expect(metrics.gainReductionDb <= target.maximumDb,
                       context);
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

        beginTest("sensitivity increases steady gain reduction");
        {
            const auto lowSensitivity = renderToSteadyState({
                50.0f, 0.0f, 3.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            });
            const auto highSensitivity = renderToSteadyState({
                50.0f, 100.0f, 3.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            });
            expect(highSensitivity.gainReductionDb
                   > lowSensitivity.gainReductionDb + 6.0f);
        }

        beginTest("ratio controls the above-reference compression slope");
        {
            const auto lowRatioAtReference = renderToSteadyState({
                50.0f, 0.0f, 2.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            }).gainReductionDb;
            const auto lowRatioLoud = renderToSteadyState({
                50.0f, 0.0f, 2.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            }, 2.0f, 2.0f).gainReductionDb;
            const auto highRatioAtReference = renderToSteadyState({
                50.0f, 0.0f, 8.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            }).gainReductionDb;
            const auto highRatioLoud = renderToSteadyState({
                50.0f, 0.0f, 8.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            }, 2.0f, 2.0f).gainReductionDb;

            expect(highRatioLoud - highRatioAtReference
                   > lowRatioLoud - lowRatioAtReference + 1.0f);
        }

        beginTest("manual makeup applies the requested wet gain");
        {
            auto settings = Voxline::Dsp::CompressorSettings {
                50.0f, 0.0f, 3.0f, 5.0f, 80.0f,
                1.0f, 0.0f, false
            };
            const auto unityMakeup = renderOutputRms(settings);
            settings.makeupDb = 3.0f;
            const auto raisedMakeup = renderOutputRms(settings);
            expectWithinAbsoluteError(
                juce::Decibels::gainToDecibels(
                    raisedMakeup / unityMakeup),
                3.0f, 0.1f,
                "unity=" + juce::String(unityMakeup)
                    + " raised=" + juce::String(raisedMakeup));
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

        beginTest("processing is invariant to runtime block segmentation");
        {
            const auto at64 = renderWithSegmentation(64);
            const auto at512 = renderWithSegmentation(512);
            const auto at2048 = renderWithSegmentation(2048);

            for (size_t sample = 0; sample < at64.size(); ++sample)
            {
                expectWithinAbsoluteError(
                    at64[sample], at512[sample], 1.0e-6f);
                expectWithinAbsoluteError(
                    at64[sample], at2048[sample], 1.0e-6f);
            }
        }

        beginTest("dense automation remains finite and continuous");
        {
            Voxline::Dsp::VocalCompressor compressor;
            compressor.prepare({testSampleRate, 64, 2});
            auto settings = Voxline::Dsp::CompressorSettings {
                75.0f, 0.0f, 3.0f, 5.0f, 80.0f,
                1.0f, 0.0f, true
            };
            compressor.setTargetSettings(settings);

            juce::AudioBuffer<float> sampleBuffer(2, 1);
            auto previous = referenceRms;
            for (int sample = 0; sample < 12000; ++sample)
            {
                if (sample > 0 && sample % 401 == 0)
                {
                    const auto phase = (sample / 401) % 3;
                    if (phase == 0)
                        settings = {
                            100.0f, 100.0f, 20.0f, 0.1f, 5.0f,
                            1.0f, 6.0f, true
                        };
                    else if (phase == 1)
                        settings = {
                            25.0f, 0.0f, 1.2f, 200.0f, 2000.0f,
                            0.2f, -6.0f, false
                        };
                    else
                        settings = {
                            0.0f, 100.0f, 20.0f, 0.1f, 5.0f,
                            1.0f, 12.0f, true
                        };
                    compressor.setTargetSettings(settings);
                }

                sampleBuffer.setSample(0, 0, referenceRms);
                sampleBuffer.setSample(1, 0, referenceRms);
                const auto metrics = compressor.process(sampleBuffer);
                const auto output = sampleBuffer.getSample(0, 0);
                expect(std::isfinite(output));
                expect(std::isfinite(metrics.gainReductionDb));
                expect(std::abs(output - previous) < 0.02f,
                       "sample=" + juce::String(sample)
                           + " previous=" + juce::String(previous)
                           + " output=" + juce::String(output)
                           + " amount=" + juce::String(settings.amount)
                           + " mix=" + juce::String(settings.mix)
                           + " makeup=" + juce::String(settings.makeupDb));
                previous = output;
            }
        }

        beginTest("repeated identical off targets finish the exact-dry fade");
        {
            for (const auto blockSize : std::array {64, 512, 2048})
                for (const auto amountOff : {false, true})
                {
                    const auto result =
                        renderRepeatedOffTarget(blockSize, amountOff);
                    expect(result.exactDryAfterTransition,
                           "block=" + juce::String(blockSize)
                               + (amountOff ? " amount" : " mix"));
                    expectEquals(result.finalReductionDb, 0.0f);
                }
        }

        beginTest("mid-transition retarget is continuous and reaches latest");
        {
            for (const auto blockSize : std::array {64, 512, 2048})
            {
                const auto result =
                    renderMidTransitionRetarget(blockSize);
                expect(result.boundaryDelta < 0.01f,
                       "block=" + juce::String(blockSize)
                           + " delta="
                           + juce::String(result.boundaryDelta));
                expectWithinAbsoluteError(
                    result.finalOutput, result.expectedOutput, 2.0e-4f,
                    "block=" + juce::String(blockSize)
                        + " expected="
                        + juce::String(result.expectedOutput)
                        + " actual="
                        + juce::String(result.finalOutput));
            }
        }

        beginTest("ratio automation to unity releases continuously to zero");
        {
            const auto at48k = renderRatioToUnity(48000.0);
            const auto at96k = renderRatioToUnity(96000.0);
            for (const auto& result : {at48k, at96k})
            {
                expect(result.reductionAt100MsDb
                       < result.startingReductionDb * 0.7f);
                expect(result.finalReductionDb < 0.05f);
                expect(result.maximumIncreaseDb < 1.0e-4f);
                expect(result.maximumSampleDelta < 0.01f);
            }
            expectWithinAbsoluteError(
                at48k.reductionAt100MsDb,
                at96k.reductionAt100MsDb,
                0.1f);
        }
    }
};

VocalCompressorTests vocalCompressorTests;
}
