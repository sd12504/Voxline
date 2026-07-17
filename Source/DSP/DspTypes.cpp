#include "DspTypes.h"

float Voxline::Dsp::gainToDbfs(float linear) noexcept
{
    return juce::Decibels::gainToDecibels(
        juce::jmax(0.0f, linear), silenceFloorDbfs);
}

float Voxline::Dsp::smoothingCoefficient(double sampleRate,
                                         float timeMs) noexcept
{
    const auto samples = static_cast<float>(
        juce::jmax(1.0, sampleRate * timeMs * 0.001));
    return std::exp(-1.0f / samples);
}
