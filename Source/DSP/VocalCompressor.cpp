#include "VocalCompressor.h"

namespace
{
constexpr float referenceLevelDbfs = -12.0f;
constexpr float kneeWidthDb = 3.0f;
constexpr float maximumAutoMakeupDb = 10.0f;
constexpr float referenceCompressionSlope = 1.0f - 1.0f / 3.0f;
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
    targetWetMix = settings.amount > 0.0f
        ? juce::jlimit(0.0f, 1.0f, settings.mix) : 0.0f;
    currentWetMix = targetWetMix;
    wetMixStep = 0.0f;
    wetTransitionRemaining = 0;
    currentAmount = settings.amount;
    currentSensitivity = settings.sensitivity;
    currentRatio = settings.ratio;
    currentMakeupDb = targetWetMix > 0.0f
        ? juce::jlimit(-12.0f, 12.0f, settings.makeupDb)
        : 0.0f;
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

    beginWetTransition(
        settings.amount > 0.0f ? settings.mix : 0.0f);
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
        advanceWetTransition();
        currentAmount = settings.amount
            + controlCoefficient * (currentAmount - settings.amount);
        currentSensitivity = settings.sensitivity
            + controlCoefficient
                * (currentSensitivity - settings.sensitivity);
        currentRatio = settings.ratio
            + controlCoefficient * (currentRatio - settings.ratio);

        if (currentWetMix <= 0.0f && targetWetMix <= 0.0f)
        {
            clearWetState();
            continue;
        }

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

        const auto compressedGain =
            juce::Decibels::decibelsToGain(
                currentMakeupDb - gainReductionDb);
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = buffer.getSample(channel, sample);
            const auto wet = dry * compressedGain;
            buffer.setSample(channel, sample,
                             dry + currentWetMix * (wet - dry));
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
        targetReductionForAmount(currentAmount);
    if (targetReduction <= 0.0f || currentRatio <= 1.0f)
        return 0.0f;

    const auto compressionSlope = 1.0f - 1.0f / currentRatio;
    const auto sensitivityOffsetDb = currentSensitivity * 0.12f;
    const auto thresholdDb = referenceLevelDbfs
        - sensitivityOffsetDb
        - targetReduction / referenceCompressionSlope;
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

void Voxline::Dsp::VocalCompressor::beginWetTransition(
    float target) noexcept
{
    const auto newTarget = juce::jlimit(0.0f, 1.0f, target);

    if (! hasProcessed)
    {
        targetWetMix = newTarget;
        currentWetMix = targetWetMix;
        wetMixStep = 0.0f;
        wetTransitionRemaining = 0;
        currentAmount = settings.amount;
        currentSensitivity = settings.sensitivity;
        currentRatio = settings.ratio;
        if (currentWetMix <= 0.0f)
            clearWetState();
        else
            currentMakeupDb = settings.makeupDb;
        return;
    }

    if (std::abs(newTarget - targetWetMix)
        <= std::numeric_limits<float>::epsilon())
        return;

    targetWetMix = newTarget;
    const auto transitionSamples = juce::jmax(
        1, juce::roundToInt(sampleRate * 0.020));
    wetTransitionRemaining = transitionSamples;
    wetMixStep = (targetWetMix - currentWetMix)
        / static_cast<float>(transitionSamples);
}

void Voxline::Dsp::VocalCompressor::advanceWetTransition() noexcept
{
    if (wetTransitionRemaining <= 0)
        return;

    currentWetMix += wetMixStep;
    --wetTransitionRemaining;

    if (wetTransitionRemaining == 0)
    {
        currentWetMix = targetWetMix;
        wetMixStep = 0.0f;
        if (currentWetMix <= 0.0f)
            clearWetState();
    }
}

void Voxline::Dsp::VocalCompressor::clearWetState() noexcept
{
    gainReductionDb = 0.0f;
    averageReductionDb = 0.0f;
    currentMakeupDb = 0.0f;
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
