#pragma once

#include "DspTypes.h"

#include <atomic>
#include <memory>

namespace Voxline::Dsp
{
struct ChannelMeter
{
    float peakDbfs {silenceFloorDbfs};
    float rmsDbfs {silenceFloorDbfs};
    float truePeakDbtp {silenceFloorDbfs};
};

struct MeterFrame
{
    std::array<ChannelMeter, 2> channels;
    int channelCount {};
    bool clipHeld {};
};

class BallisticMeter
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    MeterFrame measureBlock(const juce::AudioBuffer<float>&) noexcept;
    void clearClipHold() noexcept;

private:
    struct ChannelState
    {
        float peak {};
        float rms {};
        float truePeak {};
    };

    std::array<ChannelState, 2> channelStates;
    std::unique_ptr<juce::dsp::Oversampling<float>> truePeakOversampling;
    float peakAttackCoefficient {};
    float peakReleaseCoefficient {};
    float rmsAttackCoefficient {};
    float rmsReleaseCoefficient {};
    bool clipHeld {};
    std::atomic<bool> clearClipHoldRequested {false};
};
}
