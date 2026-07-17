#include "OutputSafety.h"

void Voxline::Dsp::EmergencySoftClipper::reset() noexcept
{
    active = false;
}

void Voxline::Dsp::EmergencySoftClipper::process(
    juce::AudioBuffer<float>& buffer) noexcept
{
    active = false;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            active = active || ! std::isfinite(samples[sample])
                     || std::abs(samples[sample]) > 0.98f;
            samples[sample] = transfer(samples[sample]);
        }
    }
}

bool Voxline::Dsp::EmergencySoftClipper::wasActive() const noexcept
{
    return active;
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
