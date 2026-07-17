#pragma once

#include "DspTypes.h"

#include <array>
#include <atomic>

namespace Voxline::Dsp
{
struct PolishSettings
{
    float amount {};
};

class VocalPolish
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const PolishSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;

private:
    struct FilterState
    {
        float presenceLowpass {};
        float airLowpass {};
    };

    std::array<FilterState, 2> filters;
    std::atomic<float> targetAmount {0.0f};
    float currentAmount {};
    float amountCoefficient {};
    float detectorEnvelope {};
    float detectorAttackCoefficient {};
    float detectorReleaseCoefficient {};
    float loudnessCoefficient {};
    float compensationCoefficient {};
    float dryPower {};
    float wetPower {};
    float compensationGain {1.0f};
    float presenceLowpassCoefficient {};
    float airLowpassCoefficient {};
    int preparedChannels {2};
};
}
