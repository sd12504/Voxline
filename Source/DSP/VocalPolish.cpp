#include "VocalPolish.h"

#include <cmath>

namespace Voxline::Dsp
{
namespace
{
float onePoleCoefficient(double sampleRate, float frequencyHz) noexcept
{
    return 1.0f
           - std::exp(-juce::MathConstants<float>::twoPi * frequencyHz
                      / static_cast<float>(sampleRate));
}
}

void VocalPolish::prepare(const ModuleSpec& spec)
{
    const auto sampleRate = juce::jmax(1.0, spec.sampleRate);
    preparedChannels = juce::jlimit(1, 2, spec.channels);
    amountCoefficient = smoothingCoefficient(sampleRate, 10.0f);
    detectorAttackCoefficient = smoothingCoefficient(sampleRate, 1.5f);
    detectorReleaseCoefficient = smoothingCoefficient(sampleRate, 80.0f);
    loudnessCoefficient = smoothingCoefficient(sampleRate, 250.0f);
    compensationCoefficient = smoothingCoefficient(sampleRate, 400.0f);
    presenceLowpassCoefficient =
        onePoleCoefficient(sampleRate, 1800.0f);
    airLowpassCoefficient = onePoleCoefficient(sampleRate, 7000.0f);
    reset();
}

void VocalPolish::reset() noexcept
{
    for (auto& filter : filters)
        filter = {};

    currentAmount = 0.0f;
    warmAmount = 0.5f;
    detectorEnvelope = 0.0f;
    dryPower = 1.0e-6f;
    wetPower = 1.0e-6f;
    compensationGain = 1.0f;
}

void VocalPolish::setTargetSettings(const PolishSettings& settings) noexcept
{
    targetAmount.store(juce::jlimit(0.0f, 1.0f, settings.amount),
                       std::memory_order_relaxed);
}

void VocalPolish::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channelCount =
        juce::jmin(preparedChannels, buffer.getNumChannels());
    const auto sampleCount = buffer.getNumSamples();
    const auto requestedAmount =
        targetAmount.load(std::memory_order_relaxed);
    std::array<float, 2> shaped {};

    if (channelCount <= 0 || sampleCount <= 0)
        return;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        if (requestedAmount > 1.0e-7f)
            warmAmount = requestedAmount;

        currentAmount =
            requestedAmount
            + amountCoefficient * (currentAmount - requestedAmount);

        if (requestedAmount <= 1.0e-7f
            && currentAmount <= 1.0e-7f)
            currentAmount = 0.0f;

        const auto processingAmount =
            requestedAmount <= 1.0e-7f ? warmAmount : currentAmount;

        float linkedDetector = 0.0f;
        for (int channel = 0; channel < channelCount; ++channel)
            linkedDetector =
                juce::jmax(linkedDetector,
                           std::abs(buffer.getSample(channel, sample)));

        const auto detectorCoefficient =
            linkedDetector > detectorEnvelope
                ? detectorAttackCoefficient
                : detectorReleaseCoefficient;
        detectorEnvelope =
            linkedDetector
            + detectorCoefficient * (detectorEnvelope - linkedDetector);

        float dynamicDb = 0.0f;
        if (detectorEnvelope > 0.20f)
            dynamicDb =
                -juce::jmin(2.0f, (detectorEnvelope - 0.20f) * 5.0f);
        else if (detectorEnvelope > 0.005f)
            dynamicDb =
                juce::jmin(1.0f, (0.20f - detectorEnvelope) * 3.0f);

        const auto transientExcess =
            juce::jmax(0.0f, linkedDetector - 0.22f);
        const auto transientGain =
            1.0f
            / (1.0f + 1.2f * processingAmount * transientExcess);
        const auto dynamicGain =
            juce::Decibels::decibelsToGain(dynamicDb * processingAmount)
            * transientGain;
        const auto saturationDrive = 1.0f + 2.5f * processingAmount;
        const auto saturationNormalisation =
            1.0f / std::tanh(saturationDrive);
        float dryInstantPower = 0.0f;
        float wetInstantPower = 0.0f;

        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto dry = buffer.getSample(channel, sample);
            auto& filter = filters[static_cast<size_t>(channel)];
            filter.presenceLowpass +=
                presenceLowpassCoefficient
                * (dry - filter.presenceLowpass);
            filter.airLowpass +=
                airLowpassCoefficient * (dry - filter.airLowpass);

            const auto highPresence = dry - filter.presenceLowpass;
            const auto highAir = dry - filter.airLowpass;
            const auto presenceBand = highPresence - highAir;
            const auto dynamicallyShaped = dry * dynamicGain;
            const auto warmed =
                std::tanh(saturationDrive * dynamicallyShaped)
                * saturationNormalisation;
            const auto enhanced =
                warmed
                + processingAmount
                      * (0.055f * presenceBand + 0.020f * highAir);
            shaped[static_cast<size_t>(channel)] = enhanced;
            dryInstantPower += dry * dry;
            wetInstantPower += enhanced * enhanced;
        }

        const auto channelScale = 1.0f / static_cast<float>(channelCount);
        dryInstantPower *= channelScale;
        wetInstantPower *= channelScale;
        dryPower =
            dryInstantPower
            + loudnessCoefficient * (dryPower - dryInstantPower);
        wetPower =
            wetInstantPower
            + loudnessCoefficient * (wetPower - wetInstantPower);

        const auto requestedCompensation =
            juce::jlimit(0.50f, 1.10f,
                         std::sqrt((dryPower + 1.0e-9f)
                                   / (wetPower + 1.0e-9f)));
        compensationGain =
            requestedCompensation
            + compensationCoefficient
                  * (compensationGain - requestedCompensation);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto dry = buffer.getSample(channel, sample);
            const auto compensated =
                shaped[static_cast<size_t>(channel)] * compensationGain;
            const auto output = currentAmount > 0.0f
                                    ? dry
                                          + currentAmount
                                                * (compensated - dry)
                                    : dry;
            buffer.setSample(channel, sample, output);
        }
    }
}
}
