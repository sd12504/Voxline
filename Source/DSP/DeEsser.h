#pragma once

#include "DspTypes.h"

#include <array>

namespace Voxline::Dsp
{
enum class DeEssMode
{
    split,
    wide
};

struct DeEsserSettings
{
    float amount {};
    float focusHz {6500.0f};
    float sensitivityDb {-18.0f};
    float maxRangeDb {6.0f};
    DeEssMode mode {DeEssMode::split};
};

struct DeEssMetrics
{
    float reductionDb {};
    bool sActive {};
};

class DeEsser
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const DeEsserSettings&) noexcept;
    DeEssMetrics process(juce::AudioBuffer<float>&) noexcept;

private:
    void updateCoefficients() noexcept;

    DeEsserSettings settings;
    double sampleRate {44100.0};
    int preparedChannels {2};

    std::array<float, 2> lowpassState {};
    std::array<float, 2> highpassSamples {};
    float detectorEnvelope {};
    float reductionEnvelopeDb {};

    float focusCoefficient {};
    float detectorAttackCoefficient {};
    float detectorReleaseCoefficient {};
    float reductionAttackCoefficient {};
    float reductionReleaseCoefficient {};
};
}
