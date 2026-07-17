#include "DeEsser.h"

namespace Voxline::Dsp
{
namespace
{
constexpr float detectorAttackMs = 0.5f;
constexpr float detectorReleaseMs = 25.0f;
constexpr float reductionAttackMs = 2.0f;
constexpr float reductionReleaseMs = 80.0f;
constexpr float inactiveReductionDb = 0.1f;
constexpr float controlTransitionMs = 10.0f;
constexpr float focusTransitionMs = 20.0f;
constexpr float detectorQ = 1.2f;
constexpr float idleReductionDb = 1.0e-4f;
}

void DeEsser::prepare(const ModuleSpec& spec)
{
    sampleRate = juce::jmax(1.0, spec.sampleRate);
    preparedChannels = juce::jlimit(1, 2, spec.channels);
    updateEnvelopeCoefficients();
    reset();
}

void DeEsser::reset() noexcept
{
    lowpassState.fill(0.0f);
    highpassSamples.fill(0.0f);
    detectorSamples.fill(0.0f);
    detectorStates.fill({});
    detectorEnvelope = 0.0f;
    reductionEnvelopeDb = 0.0f;

    currentAmount = targetAmount =
        juce::jlimit(0.0f, 1.0f, settings.amount);
    amountStep = 0.0f;
    amountTransitionRemaining = 0;

    currentModeMix = targetModeMix =
        settings.mode == DeEssMode::wide ? 1.0f : 0.0f;
    modeMixStep = 0.0f;
    modeTransitionRemaining = 0;

    const auto clampedFocusHz = juce::jlimit(
        1000.0f,
        static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)),
        settings.focusHz);
    focusCoefficient = targetFocusCoefficient = std::exp(
        -juce::MathConstants<float>::twoPi * clampedFocusHz
        / static_cast<float>(sampleRate));
    focusCoefficientStep = 0.0f;
    focusTransitionRemaining = 0;
    detectorCoefficients = detectorTargetCoefficients =
        makeDetectorCoefficients(clampedFocusHz);
    detectorCoefficientStep = {};

    idle = currentAmount <= 0.0f;
}

void DeEsser::setTargetSettings(const DeEsserSettings& target) noexcept
{
    DeEsserSettings next;
    next.amount = juce::jlimit(0.0f, 1.0f, target.amount);
    next.focusHz = juce::jlimit(
        1000.0f,
        static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)),
        target.focusHz);
    next.sensitivityDb =
        juce::jlimit(-60.0f, 0.0f, target.sensitivityDb);
    next.maxRangeDb = juce::jlimit(0.0f, 24.0f, target.maxRangeDb);
    next.mode = target.mode;

    if (std::abs(next.amount - settings.amount) > 1.0e-6f)
        beginAmountTransition(next.amount);
    if (std::abs(next.focusHz - settings.focusHz) > 1.0e-3f)
        beginFocusTransition(next.focusHz);
    if (next.mode != settings.mode)
        beginModeTransition(next.mode);

    settings = next;
}

DeEssMetrics DeEsser::process(juce::AudioBuffer<float>& buffer,
                              DeEssMonitor monitor) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    if (monitor == DeEssMonitor::off
        && targetAmount <= 0.0f
        && amountTransitionRemaining == 0
        && reductionEnvelopeDb <= idleReductionDb)
        enterIdle();

    if (idle && monitor == DeEssMonitor::off)
        return {};

    const auto channels = juce::jmin(
        preparedChannels, juce::jmin(2, buffer.getNumChannels()));
    if (channels <= 0 || buffer.getNumSamples() <= 0)
        return {reductionEnvelopeDb,
                reductionEnvelopeDb > inactiveReductionDb};

    auto* channelZero = buffer.getWritePointer(0);
    auto* channelOne = channels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto maximum = 0.0f;
        auto sumSquares = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* samples = channel == 0 ? channelZero : channelOne;
            const auto input = samples[sample];
            const auto low = input
                + focusCoefficient
                    * (lowpassState[static_cast<size_t>(channel)] - input);
            lowpassState[static_cast<size_t>(channel)] = low;
            const auto high = input - low;
            highpassSamples[static_cast<size_t>(channel)] = high;

            const auto detectorSample = processDetectorSample(
                input, detectorStates[static_cast<size_t>(channel)]);
            detectorSamples[static_cast<size_t>(channel)] = detectorSample;
            const auto magnitude = std::abs(detectorSample);
            maximum = juce::jmax(maximum, magnitude);
            sumSquares += detectorSample * detectorSample;
        }

        const auto rms = std::sqrt(
            sumSquares / static_cast<float>(channels));
        const auto detectorInput = 0.5f * (maximum + rms);
        const auto detectorCoefficient =
            detectorInput > detectorEnvelope
                ? detectorAttackCoefficient
                : detectorReleaseCoefficient;
        detectorEnvelope = detectorInput
            + detectorCoefficient * (detectorEnvelope - detectorInput);

        const auto effectiveThresholdDb = settings.sensitivityDb
            + (1.0f - currentAmount) * 12.0f;
        const auto maximumReductionDb =
            settings.maxRangeDb * currentAmount;
        const auto overThresholdDb = juce::jmax(
            0.0f, gainToDbfs(detectorEnvelope) - effectiveThresholdDb);
        const auto targetReductionDb =
            juce::jmin(maximumReductionDb, overThresholdDb);
        const auto reductionCoefficient =
            targetReductionDb > reductionEnvelopeDb
                ? reductionAttackCoefficient
                : reductionReleaseCoefficient;
        reductionEnvelopeDb = targetReductionDb
            + reductionCoefficient
                * (reductionEnvelopeDb - targetReductionDb);

        const auto linkedGain =
            juce::Decibels::decibelsToGain(-reductionEnvelopeDb);

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* samples = channel == 0 ? channelZero : channelOne;
            if (monitor == DeEssMonitor::detector)
            {
                samples[sample] =
                    detectorSamples[static_cast<size_t>(channel)];
            }
            else
            {
                const auto low =
                    lowpassState[static_cast<size_t>(channel)];
                const auto high =
                    highpassSamples[static_cast<size_t>(channel)];
                const auto splitOutput = low + high * linkedGain;
                const auto wideOutput = (low + high) * linkedGain;
                samples[sample] = splitOutput
                    + currentModeMix * (wideOutput - splitOutput);
            }
        }

        advanceControls();
    }

    if (monitor == DeEssMonitor::off
        && targetAmount <= 0.0f
        && amountTransitionRemaining == 0
        && reductionEnvelopeDb <= idleReductionDb)
        enterIdle();

    return {reductionEnvelopeDb,
            reductionEnvelopeDb > inactiveReductionDb};
}

DeEsser::BiquadCoefficients DeEsser::makeDetectorCoefficients(
    float focusHz) const noexcept
{
    const auto clampedFocusHz = juce::jlimit(
        1000.0f,
        static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)),
        focusHz);
    const auto angularFrequency =
        juce::MathConstants<float>::twoPi * clampedFocusHz
        / static_cast<float>(sampleRate);
    const auto alpha =
        std::sin(angularFrequency) / (2.0f * detectorQ);
    const auto inverseA0 = 1.0f / (1.0f + alpha);

    return {
        alpha * inverseA0,
        0.0f,
        -alpha * inverseA0,
        -2.0f * std::cos(angularFrequency) * inverseA0,
        (1.0f - alpha) * inverseA0
    };
}

float DeEsser::processDetectorSample(float input,
                                     BiquadState& state) const noexcept
{
    const auto output =
        detectorCoefficients.b0 * input
        + detectorCoefficients.b1 * state.x1
        + detectorCoefficients.b2 * state.x2
        - detectorCoefficients.a1 * state.y1
        - detectorCoefficients.a2 * state.y2;
    state.x2 = state.x1;
    state.x1 = input;
    state.y2 = state.y1;
    state.y1 = output;
    return output;
}

void DeEsser::beginAmountTransition(float target) noexcept
{
    targetAmount = target;
    const auto transitionSamples = juce::jmax(
        1, static_cast<int>(std::round(
            sampleRate * controlTransitionMs * 0.001)));
    amountTransitionRemaining = transitionSamples;
    amountStep = (targetAmount - currentAmount)
        / static_cast<float>(transitionSamples);
    if (targetAmount > 0.0f)
        idle = false;
}

void DeEsser::beginFocusTransition(float targetHz) noexcept
{
    const auto newFocusCoefficient = std::exp(
        -juce::MathConstants<float>::twoPi * targetHz
        / static_cast<float>(sampleRate));
    const auto newDetectorCoefficients =
        makeDetectorCoefficients(targetHz);

    if (idle)
    {
        focusCoefficient = targetFocusCoefficient =
            newFocusCoefficient;
        detectorCoefficients = detectorTargetCoefficients =
            newDetectorCoefficients;
        focusCoefficientStep = 0.0f;
        detectorCoefficientStep = {};
        focusTransitionRemaining = 0;
        return;
    }

    targetFocusCoefficient = newFocusCoefficient;
    detectorTargetCoefficients = newDetectorCoefficients;
    const auto transitionSamples = juce::jmax(
        1, static_cast<int>(std::round(
            sampleRate * focusTransitionMs * 0.001)));
    focusTransitionRemaining = transitionSamples;
    focusCoefficientStep =
        (targetFocusCoefficient - focusCoefficient)
        / static_cast<float>(transitionSamples);
    detectorCoefficientStep = {
        (detectorTargetCoefficients.b0 - detectorCoefficients.b0)
            / static_cast<float>(transitionSamples),
        (detectorTargetCoefficients.b1 - detectorCoefficients.b1)
            / static_cast<float>(transitionSamples),
        (detectorTargetCoefficients.b2 - detectorCoefficients.b2)
            / static_cast<float>(transitionSamples),
        (detectorTargetCoefficients.a1 - detectorCoefficients.a1)
            / static_cast<float>(transitionSamples),
        (detectorTargetCoefficients.a2 - detectorCoefficients.a2)
            / static_cast<float>(transitionSamples)
    };
}

void DeEsser::beginModeTransition(DeEssMode target) noexcept
{
    targetModeMix = target == DeEssMode::wide ? 1.0f : 0.0f;
    if (idle)
    {
        currentModeMix = targetModeMix;
        modeMixStep = 0.0f;
        modeTransitionRemaining = 0;
        return;
    }

    const auto transitionSamples = juce::jmax(
        1, static_cast<int>(std::round(
            sampleRate * controlTransitionMs * 0.001)));
    modeTransitionRemaining = transitionSamples;
    modeMixStep = (targetModeMix - currentModeMix)
        / static_cast<float>(transitionSamples);
}

void DeEsser::advanceControls() noexcept
{
    if (amountTransitionRemaining > 0)
    {
        currentAmount += amountStep;
        if (--amountTransitionRemaining == 0)
            currentAmount = targetAmount;
    }

    if (modeTransitionRemaining > 0)
    {
        currentModeMix += modeMixStep;
        if (--modeTransitionRemaining == 0)
            currentModeMix = targetModeMix;
    }

    if (focusTransitionRemaining > 0)
    {
        focusCoefficient += focusCoefficientStep;
        detectorCoefficients.b0 += detectorCoefficientStep.b0;
        detectorCoefficients.b1 += detectorCoefficientStep.b1;
        detectorCoefficients.b2 += detectorCoefficientStep.b2;
        detectorCoefficients.a1 += detectorCoefficientStep.a1;
        detectorCoefficients.a2 += detectorCoefficientStep.a2;
        if (--focusTransitionRemaining == 0)
        {
            focusCoefficient = targetFocusCoefficient;
            detectorCoefficients = detectorTargetCoefficients;
        }
    }
}

void DeEsser::enterIdle() noexcept
{
    idle = true;
    lowpassState.fill(0.0f);
    highpassSamples.fill(0.0f);
    detectorSamples.fill(0.0f);
    detectorStates.fill({});
    detectorEnvelope = 0.0f;
    reductionEnvelopeDb = 0.0f;
}

void DeEsser::updateEnvelopeCoefficients() noexcept
{
    detectorAttackCoefficient =
        smoothingCoefficient(sampleRate, detectorAttackMs);
    detectorReleaseCoefficient =
        smoothingCoefficient(sampleRate, detectorReleaseMs);
    reductionAttackCoefficient =
        smoothingCoefficient(sampleRate, reductionAttackMs);
    reductionReleaseCoefficient =
        smoothingCoefficient(sampleRate, reductionReleaseMs);
}
}
