#pragma once

#include "DspTypes.h"

#include <atomic>

namespace Voxline::Dsp
{
class EmergencySoftClipper
{
public:
    void reset() noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    bool wasActive() const noexcept;
    static float transfer(float sample) noexcept;

private:
    std::atomic<bool> activeSnapshot {false};
};
}
