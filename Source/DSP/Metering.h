#pragma once

#include "DspTypes.h"

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
        std::array<float, 2> previousSamples {};
        int previousSampleCount {};
    };

    static float measureTruePeak(const float*,
                                 int,
                                 ChannelState&,
                                 float) noexcept;

    std::array<ChannelState, 2> channelStates;
    float peakAttackCoefficient {};
    float peakReleaseCoefficient {};
    float rmsAttackCoefficient {};
    float rmsReleaseCoefficient {};
    bool clipHeld {};
};
}
