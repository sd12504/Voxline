#include "OutputSafety.h"

void Voxline::Dsp::EmergencySoftClipper::reset() noexcept
{
    activeSnapshot.store(false, std::memory_order_release);
}

void Voxline::Dsp::EmergencySoftClipper::process(
    juce::AudioBuffer<float>& buffer) noexcept
{
    auto blockActive = false;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            blockActive =
                blockActive || ! std::isfinite(samples[sample])
                || std::abs(samples[sample]) > 0.98f;
            samples[sample] = transfer(samples[sample]);
        }
    }

    activeSnapshot.store(blockActive, std::memory_order_release);
}

bool Voxline::Dsp::EmergencySoftClipper::wasActive() const noexcept
{
    return activeSnapshot.load(std::memory_order_acquire);
}

float Voxline::Dsp::EmergencySoftClipper::transfer(float sample) noexcept
{
    constexpr float threshold = 0.98f;

    if (! std::isfinite(sample))
        return std::isnan(sample) ? 0.0f : std::copysign(1.0f, sample);

    const auto magnitude = std::abs(sample);
    if (magnitude <= threshold)
        return sample;

    const auto normalised =
        (magnitude - threshold) / (1.0f - threshold);
    const auto curved =
        threshold
        + (1.0f - threshold) * (1.0f - std::exp(-normalised));
    return std::copysign(juce::jmin(curved, 1.0f), sample);
}
