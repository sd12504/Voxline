#include "Metering.h"

namespace
{
float smoothBlock(float current,
                  float target,
                  float attackCoefficient,
                  float releaseCoefficient,
                  int numSamples) noexcept
{
    if (numSamples <= 0)
        return current;

    const auto perSampleCoefficient =
        target > current ? attackCoefficient : releaseCoefficient;
    const auto blockCoefficient =
        std::pow(perSampleCoefficient, static_cast<float>(numSamples));
    return target + blockCoefficient * (current - target);
}

float cubicInterpolate(float p0,
                       float p1,
                       float p2,
                       float p3,
                       float position) noexcept
{
    const auto positionSquared = position * position;
    const auto positionCubed = positionSquared * position;
    return 0.5f
           * ((2.0f * p1)
              + (-p0 + p2) * position
              + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3)
                    * positionSquared
              + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * positionCubed);
}

}

void Voxline::Dsp::BallisticMeter::prepare(const ModuleSpec& spec)
{
    const auto sampleRate = juce::jmax(1.0, spec.sampleRate);
    peakAttackCoefficient = smoothingCoefficient(sampleRate, 8.0f);
    peakReleaseCoefficient = smoothingCoefficient(sampleRate, 400.0f);
    rmsAttackCoefficient = smoothingCoefficient(sampleRate, 80.0f);
    rmsReleaseCoefficient = smoothingCoefficient(sampleRate, 600.0f);
    reset();
}

void Voxline::Dsp::BallisticMeter::reset() noexcept
{
    channelStates = {};
    clipHeld = false;
}

float Voxline::Dsp::BallisticMeter::measureTruePeak(
    const float* samples,
    int numSamples,
    ChannelState& state,
    float samplePeak) noexcept
{
    auto truePeak = samplePeak;

    if (state.previousSampleCount == 2 && numSamples > 0)
    {
        const auto p0 = state.previousSamples[0];
        const auto p1 = state.previousSamples[1];
        const auto p2 = samples[0];
        const auto p3 = numSamples > 1 ? samples[1] : p2;

        for (int phase = 1; phase < 4; ++phase)
        {
            const auto interpolated =
                cubicInterpolate(p0, p1, p2, p3,
                                 static_cast<float>(phase) * 0.25f);
            truePeak = juce::jmax(truePeak, std::abs(interpolated));
        }
    }

    for (int sample = 0; sample + 1 < numSamples; ++sample)
    {
        const auto p0 =
            sample > 0
                ? samples[sample - 1]
                : (state.previousSampleCount > 0
                       ? state.previousSamples[1]
                       : samples[sample]);
        const auto p1 = samples[sample];
        const auto p2 = samples[sample + 1];
        const auto p3 =
            sample + 2 < numSamples ? samples[sample + 2] : p2;

        for (int phase = 1; phase < 4; ++phase)
        {
            const auto interpolated =
                cubicInterpolate(p0, p1, p2, p3,
                                 static_cast<float>(phase) * 0.25f);
            truePeak = juce::jmax(truePeak, std::abs(interpolated));
        }
    }

    if (numSamples >= 2)
    {
        state.previousSamples[0] = samples[numSamples - 2];
        state.previousSamples[1] = samples[numSamples - 1];
        state.previousSampleCount = 2;
    }
    else if (numSamples == 1)
    {
        state.previousSamples[0] = state.previousSamples[1];
        state.previousSamples[1] = samples[0];
        state.previousSampleCount =
            juce::jmin(2, state.previousSampleCount + 1);
    }

    return truePeak;
}

Voxline::Dsp::MeterFrame Voxline::Dsp::BallisticMeter::measureBlock(
    const juce::AudioBuffer<float>& buffer) noexcept
{
    MeterFrame frame;
    frame.channelCount = juce::jlimit(0, 2, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            if (std::abs(samples[sample]) >= 1.0f)
                clipHeld = true;
    }

    for (int channel = 0; channel < frame.channelCount; ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        auto samplePeak = 0.0f;
        auto sumOfSquares = 0.0;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto value = samples[sample];
            samplePeak = juce::jmax(samplePeak, std::abs(value));
            sumOfSquares += static_cast<double>(value)
                            * static_cast<double>(value);
        }

        const auto blockRms =
            numSamples > 0
                ? static_cast<float>(
                      std::sqrt(sumOfSquares / static_cast<double>(numSamples)))
                : channelStates[static_cast<size_t>(channel)].rms;
        auto& state = channelStates[static_cast<size_t>(channel)];
        const auto truePeak =
            measureTruePeak(samples, numSamples, state, samplePeak);

        state.peak =
            smoothBlock(state.peak, samplePeak, peakAttackCoefficient,
                        peakReleaseCoefficient, numSamples);
        state.rms =
            smoothBlock(state.rms, blockRms, rmsAttackCoefficient,
                        rmsReleaseCoefficient, numSamples);
        state.truePeak =
            smoothBlock(state.truePeak, truePeak, peakAttackCoefficient,
                        peakReleaseCoefficient, numSamples);

        auto& output = frame.channels[static_cast<size_t>(channel)];
        output.peakDbfs = gainToDbfs(state.peak);
        output.rmsDbfs = gainToDbfs(state.rms);
        output.truePeakDbtp = gainToDbfs(state.truePeak);
    }

    frame.clipHeld = clipHeld;
    return frame;
}

void Voxline::Dsp::BallisticMeter::clearClipHold() noexcept
{
    clipHeld = false;
}
