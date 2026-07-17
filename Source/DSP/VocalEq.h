#pragma once

#include "DspTypes.h"

#include <memory>

namespace Voxline::Dsp
{
enum class FilterSlope
{
    db12,
    db24,
    db36,
    db48
};

struct EqBandSettings
{
    bool enabled {};
    float frequencyHz {};
    float gainDb {};
    float q {1.0f};
};

struct VocalEqSettings
{
    bool enabled {true};
    EqBandSettings hpf {false, 80.0f, 0.0f, 0.707f};
    EqBandSettings low {true, 160.0f, 0.0f, 0.8f};
    EqBandSettings mud {false, 350.0f, 0.0f, 1.1f};
    EqBandSettings presence {true, 2500.0f, 0.0f, 1.0f};
    EqBandSettings air {true, 10000.0f, 0.0f, 0.7f};
    EqBandSettings lpf {false, 18000.0f, 0.0f, 0.707f};
    FilterSlope hpfSlope {FilterSlope::db24};
    FilterSlope lpfSlope {FilterSlope::db12};
};

class VocalEq
{
public:
    VocalEq();
    ~VocalEq();

    VocalEq(const VocalEq&) = delete;
    VocalEq& operator=(const VocalEq&) = delete;

    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const VocalEqSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    float magnitudeAt(float frequencyHz) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
