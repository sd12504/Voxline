#pragma once

#include "DspTypes.h"

namespace Voxline::Dsp
{
struct CompressorSettings
{
    float amount {};
    float sensitivity {};
    float ratio {3.0f};
    float attackMs {15.0f};
    float releaseMs {80.0f};
    float mix {1.0f};
    float makeupDb {};
    bool autoMakeup {true};
};

struct CompressorMetrics
{
    float gainReductionDb {};
};

class VocalCompressor
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const CompressorSettings&) noexcept;
    CompressorMetrics process(juce::AudioBuffer<float>&) noexcept;

private:
    static float targetReductionForAmount(float amount) noexcept;
    float calculateReduction(float detectorDb) const noexcept;
    void updateCoefficients() noexcept;

    CompressorSettings settings;
    double sampleRate {44100.0};
    int preparedChannels {2};

    float detectorPower {};
    float gainReductionDb {};
    float averageReductionDb {};
    float currentMix {1.0f};
    float currentMakeupDb {};

    float detectorCoefficient {};
    float attackCoefficient {};
    float releaseCoefficient {};
    float averageCoefficient {};
    float controlCoefficient {};
    bool hasProcessed {};
};
}
