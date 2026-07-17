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

}

void Voxline::Dsp::BallisticMeter::prepare(const ModuleSpec& spec)
{
    const auto sampleRate = juce::jmax(1.0, spec.sampleRate);
    peakAttackCoefficient = smoothingCoefficient(sampleRate, 8.0f);
    peakReleaseCoefficient = smoothingCoefficient(sampleRate, 400.0f);
    rmsAttackCoefficient = smoothingCoefficient(sampleRate, 80.0f);
    rmsReleaseCoefficient = smoothingCoefficient(sampleRate, 600.0f);

    truePeakOversampling =
        std::make_unique<juce::dsp::Oversampling<float>>(
            2,
            2,
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true);
    truePeakOversampling->initProcessing(
        static_cast<size_t>(juce::jmax(1, spec.maximumBlockSize)));
    reset();
}

void Voxline::Dsp::BallisticMeter::reset() noexcept
{
    channelStates = {};
    clipHeld = false;
    clearClipHoldRequested.store(false, std::memory_order_relaxed);

    if (truePeakOversampling != nullptr)
        truePeakOversampling->reset();
}

Voxline::Dsp::MeterFrame Voxline::Dsp::BallisticMeter::measureBlock(
    const juce::AudioBuffer<float>& buffer) noexcept
{
    if (clearClipHoldRequested.exchange(false, std::memory_order_acq_rel))
        clipHeld = false;

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

    juce::dsp::AudioBlock<float> oversampledBlock;
    if (truePeakOversampling != nullptr && numSamples > 0)
    {
        const juce::dsp::AudioBlock<const float> inputBlock(buffer);
        oversampledBlock =
            truePeakOversampling->processSamplesUp(inputBlock);
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
        auto truePeak = samplePeak;

        if (static_cast<size_t>(channel) < oversampledBlock.getNumChannels())
        {
            const auto* oversampled =
                oversampledBlock.getChannelPointer(
                    static_cast<size_t>(channel));
            for (size_t sample = 0;
                 sample < oversampledBlock.getNumSamples();
                 ++sample)
                truePeak =
                    juce::jmax(truePeak, std::abs(oversampled[sample]));
        }

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
    clearClipHoldRequested.store(true, std::memory_order_release);
}
