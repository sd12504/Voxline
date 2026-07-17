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
}

void DeEsser::prepare(const ModuleSpec& spec)
{
    sampleRate = juce::jmax(1.0, spec.sampleRate);
    preparedChannels = juce::jlimit(1, 2, spec.channels);
    updateCoefficients();
    reset();
}

void DeEsser::reset() noexcept
{
    lowpassState.fill(0.0f);
    highpassSamples.fill(0.0f);
    detectorEnvelope = 0.0f;
    reductionEnvelopeDb = 0.0f;
}

void DeEsser::setTargetSettings(const DeEsserSettings& target) noexcept
{
    settings.amount = juce::jlimit(0.0f, 1.0f, target.amount);
    settings.focusHz = juce::jlimit(
        1000.0f,
        static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)),
        target.focusHz);
    settings.sensitivityDb = juce::jlimit(-60.0f, 0.0f,
                                          target.sensitivityDb);
    settings.maxRangeDb = juce::jlimit(0.0f, 24.0f, target.maxRangeDb);
    settings.mode = target.mode;
    updateCoefficients();
}

DeEssMetrics DeEsser::process(juce::AudioBuffer<float>& buffer) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    if (settings.amount <= 0.0f)
    {
        detectorEnvelope = 0.0f;
        reductionEnvelopeDb = 0.0f;
        lowpassState.fill(0.0f);
        return {};
    }

    const auto channels = juce::jmin(
        preparedChannels, juce::jmin(2, buffer.getNumChannels()));
    if (channels <= 0 || buffer.getNumSamples() <= 0)
        return {reductionEnvelopeDb,
                reductionEnvelopeDb > inactiveReductionDb};

    auto* channelZero = buffer.getWritePointer(0);
    auto* channelOne = channels > 1 ? buffer.getWritePointer(1) : nullptr;

    const auto effectiveThresholdDb = settings.sensitivityDb
        + (1.0f - settings.amount) * 12.0f;
    const auto maximumReductionDb = settings.maxRangeDb * settings.amount;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto maximum = 0.0f;
        auto sumSquares = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* samples = channel == 0 ? channelZero : channelOne;
            const auto input = samples[sample];
            const auto low = input
                + focusCoefficient * (lowpassState[static_cast<size_t>(channel)]
                                      - input);
            lowpassState[static_cast<size_t>(channel)] = low;
            const auto high = input - low;
            highpassSamples[static_cast<size_t>(channel)] = high;

            const auto magnitude = std::abs(high);
            maximum = juce::jmax(maximum, magnitude);
            sumSquares += high * high;
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

        const auto overThresholdDb = juce::jmax(
            0.0f, gainToDbfs(detectorEnvelope) - effectiveThresholdDb);
        const auto targetReductionDb = juce::jmin(
            maximumReductionDb, overThresholdDb * settings.amount);
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
            if (settings.mode == DeEssMode::wide)
            {
                samples[sample] *= linkedGain;
            }
            else
            {
                const auto high =
                    highpassSamples[static_cast<size_t>(channel)];
                samples[sample] = lowpassState[static_cast<size_t>(channel)]
                    + high * linkedGain;
            }
        }
    }

    return {reductionEnvelopeDb,
            reductionEnvelopeDb > inactiveReductionDb};
}

void DeEsser::updateCoefficients() noexcept
{
    const auto clampedFocusHz = juce::jlimit(
        1000.0f,
        static_cast<float>(juce::jmin(20000.0, sampleRate * 0.45)),
        settings.focusHz);
    focusCoefficient = std::exp(
        -juce::MathConstants<float>::twoPi * clampedFocusHz
        / static_cast<float>(sampleRate));
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
