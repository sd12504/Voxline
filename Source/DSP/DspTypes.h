#pragma once

#include <JuceHeader.h>

namespace Voxline::Dsp
{
inline constexpr float silenceFloorDbfs = -60.0f;

struct ModuleSpec
{
    double sampleRate {44100.0};
    int maximumBlockSize {512};
    int channels {2};
};

float gainToDbfs(float linear) noexcept;
float smoothingCoefficient(double sampleRate, float timeMs) noexcept;
}
