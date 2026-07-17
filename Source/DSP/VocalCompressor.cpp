#include "VocalCompressor.h"

namespace
{
constexpr float referenceLevelDbfs = -12.0f;
constexpr float kneeWidthDb = 3.0f;
constexpr float maximumAutoMakeupDb = 10.0f;
}

void Voxline::Dsp::VocalCompressor::prepare(const ModuleSpec& spec)
{
    sampleRate = juce::jmax(1.0, spec.sampleRate);
    preparedChannels = juce::jlimit(1, 2, spec.channels);
    updateCoefficients();
    reset();
}

void Voxline::Dsp::VocalCompressor::reset() noexcept
{
    detectorPower = 0.0f;
    gainReductionDb = 0.0f;
    averageReductionDb = 0.0f;
    currentMix = juce::jlimit(0.0f, 1.0f, settings.mix);
    currentMakeupDb = juce::jlimit(-12.0f, 12.0f,
                                   settings.makeupDb);
    hasProcessed = false;
}

void Voxline::Dsp::VocalCompressor::setTargetSettings(
    const CompressorSettings& target) noexcept
{
    settings.amount = juce::jlimit(0.0f, 100.0f, target.amount);
    settings.sensitivity =
        juce::jlimit(0.0f, 100.0f, target.sensitivity);
    settings.ratio = juce::jlimit(1.0f, 20.0f, target.ratio);
    settings.attackMs = juce::jlimit(0.1f, 200.0f, target.attackMs);
    settings.releaseMs =
        juce::jlimit(5.0f, 2000.0f, target.releaseMs);
    settings.mix = juce::jlimit(0.0f, 1.0f, target.mix);
    settings.makeupDb =
        juce::jlimit(-12.0f, 12.0f, target.makeupDb);
    settings.autoMakeup = target.autoMakeup;
    updateCoefficients();

    if (! hasProcessed)
    {
        currentMix = settings.mix;
        currentMakeupDb = settings.makeupDb;
    }
}

Voxline::Dsp::CompressorMetrics
Voxline::Dsp::VocalCompressor::process(
    juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin(
        preparedChannels, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();

    if (channels <= 0 || samples <= 0)
        return {gainReductionDb};

    for (int sample = 0; sample < samples; ++sample)
    {
        auto linkedPower = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto value = buffer.getSample(channel, sample);
            linkedPower += value * value;
        }
        linkedPower /= static_cast<float>(channels);

        detectorPower = linkedPower
            + detectorCoefficient * (detectorPower - linkedPower);
        const auto detectorDb = 10.0f * std::log10(
            juce::jmax(detectorPower, 1.0e-12f));
        const auto targetReduction = calculateReduction(detectorDb);
        const auto envelopeCoefficient =
            targetReduction > gainReductionDb
                ? attackCoefficient : releaseCoefficient;
        gainReductionDb = targetReduction
            + envelopeCoefficient
                * (gainReductionDb - targetReduction);

        averageReductionDb = gainReductionDb
            + averageCoefficient
                * (averageReductionDb - gainReductionDb);
        const auto autoMakeupDb = settings.autoMakeup
            ? juce::jlimit(0.0f, maximumAutoMakeupDb,
                           averageReductionDb)
            : 0.0f;
        const auto targetMakeupDb =
            juce::jlimit(-12.0f, 22.0f,
                         settings.makeupDb + autoMakeupDb);
        currentMakeupDb = targetMakeupDb
            + controlCoefficient
                * (currentMakeupDb - targetMakeupDb);
        currentMix = settings.mix
            + controlCoefficient * (currentMix - settings.mix);

        if (currentMix <= 0.0f)
            continue;

        const auto compressedGain =
            juce::Decibels::decibelsToGain(
                currentMakeupDb - gainReductionDb);
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = buffer.getSample(channel, sample);
            const auto wet = dry * compressedGain;
            buffer.setSample(channel, sample,
                             dry + currentMix * (wet - dry));
        }
    }

    hasProcessed = true;
    return {juce::jmax(0.0f, gainReductionDb)};
}

float Voxline::Dsp::VocalCompressor::targetReductionForAmount(
    float amount) noexcept
{
    const auto normalised =
        juce::jlimit(0.0f, 100.0f, amount) * 0.01f;

    if (normalised <= 0.25f)
        return normalised * 6.0f;
    if (normalised <= 0.5f)
        return 1.5f + (normalised - 0.25f) * 8.0f;
    if (normalised <= 0.75f)
        return 3.5f + (normalised - 0.5f) * 10.0f;
    return 6.0f + (normalised - 0.75f) * 12.0f;
}

float Voxline::Dsp::VocalCompressor::calculateReduction(
    float detectorDb) const noexcept
{
    const auto targetReduction =
        targetReductionForAmount(settings.amount);
    if (targetReduction <= 0.0f || settings.ratio <= 1.0f)
        return 0.0f;

    const auto compressionSlope = 1.0f - 1.0f / settings.ratio;
    const auto sensitivityOffsetDb = settings.sensitivity * 0.12f;
    const auto thresholdDb = referenceLevelDbfs
        - sensitivityOffsetDb - targetReduction / compressionSlope;
    const auto overThresholdDb = detectorDb - thresholdDb;
    const auto halfKneeDb = kneeWidthDb * 0.5f;

    if (overThresholdDb <= -halfKneeDb)
        return 0.0f;
    if (overThresholdDb >= halfKneeDb)
        return compressionSlope * overThresholdDb;

    const auto kneePosition = overThresholdDb + halfKneeDb;
    return compressionSlope * kneePosition * kneePosition
        / (2.0f * kneeWidthDb);
}

void Voxline::Dsp::VocalCompressor::updateCoefficients() noexcept
{
    detectorCoefficient = smoothingCoefficient(
        sampleRate, 10.0f);
    attackCoefficient = smoothingCoefficient(
        sampleRate, settings.attackMs);
    releaseCoefficient = smoothingCoefficient(
        sampleRate, settings.releaseMs);
    averageCoefficient = smoothingCoefficient(
        sampleRate, 500.0f);
    controlCoefficient = smoothingCoefficient(
        sampleRate, 20.0f);
}
