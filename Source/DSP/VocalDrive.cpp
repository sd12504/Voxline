#include "VocalDrive.h"

#include <cmath>

namespace Voxline::Dsp
{
namespace
{
constexpr size_t oversamplingStages = 2;
constexpr float parameterSmoothingMs = 10.0f;
constexpr float levelMatchSmoothingMs = 50.0f;
constexpr float characterCrossfadeMs = 10.0f;
constexpr float toneFrequencyHz = 4500.0f;

float sanitise(float value, float fallback = 0.0f) noexcept
{
    return std::isfinite(value) ? value : fallback;
}
}

void VocalDrive::prepare(const ModuleSpec& spec)
{
    moduleSpec.sampleRate = juce::jmax(1.0, spec.sampleRate);
    moduleSpec.maximumBlockSize = juce::jmax(1, spec.maximumBlockSize);
    moduleSpec.channels = juce::jlimit(1, 2, spec.channels);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(moduleSpec.channels),
        oversamplingStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);
    oversampling->initProcessing(
        static_cast<size_t>(moduleSpec.maximumBlockSize));
    latency = juce::roundToInt(oversampling->getLatencyInSamples());

    delayedDry.setSize(moduleSpec.channels,
                       moduleSpec.maximumBlockSize,
                       false,
                       true,
                       false);
    const auto delayCapacity =
        juce::jmax(1, latency + moduleSpec.maximumBlockSize + 1);
    dryDelay.assign(static_cast<size_t>(moduleSpec.channels),
                    std::vector<float>(static_cast<size_t>(delayCapacity), 0.0f));
    toneState.assign(static_cast<size_t>(moduleSpec.channels), 0.0f);

    const auto oversampledRate =
        moduleSpec.sampleRate * static_cast<double>(1u << oversamplingStages);
    parameterCoefficient =
        smoothingCoefficient(oversampledRate, parameterSmoothingMs);
    baseParameterCoefficient =
        smoothingCoefficient(moduleSpec.sampleRate, parameterSmoothingMs);
    matchCoefficient =
        smoothingCoefficient(moduleSpec.sampleRate, levelMatchSmoothingMs);
    toneCoefficient = static_cast<float>(std::exp(
        -juce::MathConstants<double>::twoPi * toneFrequencyHz
        / oversampledRate));

    prepared = true;
    reset();
}

void VocalDrive::reset() noexcept
{
    if (oversampling != nullptr)
        oversampling->reset();

    delayedDry.clear();
    for (auto& channel : dryDelay)
        std::fill(channel.begin(), channel.end(), 0.0f);
    std::fill(toneState.begin(), toneState.end(), 0.0f);

    currentAmount = targetSettings.amount;
    currentTone = targetSettings.tone;
    currentMix = targetSettings.mix;
    currentTrimDb = targetSettings.outputTrimDb;
    currentMatchGain = 1.0f;
    targetMatchGain = 1.0f;
    currentLevelMatchWeight = targetSettings.levelMatch ? 1.0f : 0.0f;
    currentWetEnable = targetSettings.amount > 0.0f ? 1.0f : 0.0f;
    previousCharacter = targetSettings.character;
    currentCharacter = targetSettings.character;
    characterFade = 1.0f;
    dryWritePosition = 0;
}

void VocalDrive::setTargetSettings(const DriveSettings& settings) noexcept
{
    DriveSettings safe = settings;
    safe.amount = juce::jlimit(0.0f, 1.0f, sanitise(safe.amount));
    safe.tone = juce::jlimit(-1.0f, 1.0f, sanitise(safe.tone));
    safe.mix = juce::jlimit(0.0f, 1.0f, sanitise(safe.mix, 0.7f));
    safe.outputTrimDb =
        juce::jlimit(-24.0f, 12.0f, sanitise(safe.outputTrimDb));

    if (safe.character != currentCharacter)
    {
        previousCharacter = currentCharacter;
        currentCharacter = safe.character;
        characterFade = 0.0f;
    }

    targetSettings = safe;
}

void VocalDrive::process(juce::AudioBuffer<float>& audio) noexcept
{
    if (! prepared || oversampling == nullptr
        || audio.getNumSamples() <= 0 || audio.getNumChannels() <= 0)
        return;

    const auto channels = juce::jmin(audio.getNumChannels(), moduleSpec.channels);
    const auto samples = juce::jmin(audio.getNumSamples(),
                                    moduleSpec.maximumBlockSize);
    const auto delayCapacity =
        static_cast<int>(dryDelay.front().size());
    auto inputEnergy = 0.0;

    for (int sample = 0; sample < samples; ++sample)
    {
        auto readPosition = dryWritePosition - latency;
        if (readPosition < 0)
            readPosition += delayCapacity;

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto input = audio.getSample(channel, sample);
            inputEnergy += static_cast<double>(input) * input;
            delayedDry.setSample(
                channel,
                sample,
                latency == 0
                    ? input
                    : dryDelay[static_cast<size_t>(channel)]
                              [static_cast<size_t>(readPosition)]);
            dryDelay[static_cast<size_t>(channel)]
                    [static_cast<size_t>(dryWritePosition)] = input;
        }

        if (++dryWritePosition == delayCapacity)
            dryWritePosition = 0;
    }

    juce::dsp::AudioBlock<float> audioBlock(audio);
    const juce::dsp::AudioBlock<const float> inputBlock(audioBlock);
    auto oversampled = oversampling->processSamplesUp(inputBlock);
    const auto fadeStep = static_cast<float>(
        1.0 / juce::jmax(
            1.0,
            moduleSpec.sampleRate
                * static_cast<double>(1u << oversamplingStages)
                * characterCrossfadeMs
                * 0.001));

    for (size_t sample = 0; sample < oversampled.getNumSamples(); ++sample)
    {
        currentAmount =
            advance(currentAmount, targetSettings.amount, parameterCoefficient);
        currentTone =
            advance(currentTone, targetSettings.tone, parameterCoefficient);
        characterFade = juce::jmin(1.0f, characterFade + fadeStep);

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* samplesForChannel =
                oversampled.getChannelPointer(static_cast<size_t>(channel));
            const auto input = samplesForChannel[sample];
            const auto previous =
                transfer(input, currentAmount, previousCharacter);
            const auto next =
                transfer(input, currentAmount, currentCharacter);
            auto driven = previous + characterFade * (next - previous);

            auto& low = toneState[static_cast<size_t>(channel)];
            low = (1.0f - toneCoefficient) * driven
                + toneCoefficient * low;
            if (currentTone >= 0.0f)
                driven += currentTone * 0.45f * (driven - low);
            else
                driven += -currentTone * 0.75f * (low - driven);

            samplesForChannel[sample] = driven;
        }

        if (characterFade >= 1.0f)
            previousCharacter = currentCharacter;
    }

    oversampling->processSamplesDown(audioBlock);

    auto wetEnergy = 0.0;
    for (int channel = 0; channel < channels; ++channel)
        for (int sample = 0; sample < samples; ++sample)
        {
            const auto wet = audio.getSample(channel, sample);
            wetEnergy += static_cast<double>(wet) * wet;
        }

    if (inputEnergy > 1.0e-12 && wetEnergy > 1.0e-12)
    {
        targetMatchGain = juce::jlimit(
            0.03f,
            4.0f,
            static_cast<float>(std::sqrt(inputEnergy / wetEnergy)));
    }
    else
    {
        targetMatchGain = 1.0f;
    }

    for (int sample = 0; sample < samples; ++sample)
    {
        currentMatchGain =
            advance(currentMatchGain, targetMatchGain, matchCoefficient);
        currentLevelMatchWeight =
            advance(currentLevelMatchWeight,
                    targetSettings.levelMatch ? 1.0f : 0.0f,
                    baseParameterCoefficient);
        currentWetEnable =
            advance(currentWetEnable,
                    targetSettings.amount > 0.0f ? 1.0f : 0.0f,
                    baseParameterCoefficient);
        currentMix =
            advance(currentMix,
                    targetSettings.mix,
                    baseParameterCoefficient);
        currentTrimDb =
            advance(currentTrimDb,
                    targetSettings.outputTrimDb,
                    baseParameterCoefficient);
        const auto trimGain =
            juce::Decibels::decibelsToGain(currentTrimDb);

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = delayedDry.getSample(channel, sample);
            const auto matchGain =
                1.0f
                + currentLevelMatchWeight * (currentMatchGain - 1.0f);
            const auto wet = audio.getSample(channel, sample) * matchGain;
            const auto processed =
                (dry + currentMix * (wet - dry)) * trimGain;
            const auto output =
                dry + currentWetEnable * (processed - dry);
            audio.setSample(channel, sample,
                            std::isfinite(output) ? output : 0.0f);
        }
    }
}

int VocalDrive::latencySamples() const noexcept
{
    return latency;
}

float VocalDrive::transfer(float sample,
                           float amount,
                           DriveCharacter character) noexcept
{
    if (amount <= 0.0f)
        return sample;

    const auto preGain = 1.0f + amount * amount * 14.0f;
    const auto input = juce::jlimit(-32.0f, 32.0f, sample * preGain);

    switch (character)
    {
        case DriveCharacter::clean:
            return std::tanh(input);

        case DriveCharacter::warm:
        {
            const auto biased = input + 0.18f * amount;
            const auto shaped = std::tanh(
                biased * (input >= 0.0f ? 0.82f : 1.08f));
            return shaped - std::tanh(0.18f * amount);
        }

        case DriveCharacter::edge:
        {
            const auto asymmetric =
                input + 0.22f * amount * input * input
                      / (1.0f + std::abs(input));
            const auto clipped =
                juce::jlimit(-1.25f, 0.92f, asymmetric);
            return 0.72f * std::tanh(clipped * 1.7f)
                 + 0.28f * (2.0f / juce::MathConstants<float>::pi)
                         * std::atan(clipped * 4.0f);
        }
    }

    return sample;
}

float VocalDrive::advance(float current,
                          float target,
                          float coefficient) noexcept
{
    const auto next = target + coefficient * (current - target);
    return std::abs(next - target) < 1.0e-7f ? target : next;
}
}
