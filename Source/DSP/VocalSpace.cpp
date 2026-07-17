#include "VocalSpace.h"

#include <algorithm>
#include <cmath>

namespace Voxline::Dsp
{
namespace
{
constexpr float parameterSmoothingMs = 10.0f;
constexpr float tapCrossfadeMs = 20.0f;
constexpr float modeCrossfadeMs = 30.0f;
constexpr float duckAttackMs = 5.0f;
constexpr float duckReleaseMs = 180.0f;
constexpr float maximumPreDelayMs = 250.0f;

constexpr std::array<float, 4> roomDelaySeconds {
    0.0113f, 0.0179f, 0.0297f, 0.0371f
};
constexpr std::array<float, 4> plateDelaySeconds {
    0.0297f, 0.0371f, 0.0411f, 0.0533f
};
constexpr std::array<float, 8> hallDelaySeconds {
    0.0371f, 0.0533f, 0.0719f, 0.0899f,
    0.113f, 0.149f, 0.193f, 0.241f
};
constexpr std::array<float, 4> plateDiffuserLeftMs {
    3.1f, 4.7f, 7.3f, 11.1f
};
constexpr std::array<float, 4> plateDiffuserRightMs {
    3.7f, 5.3f, 8.1f, 12.7f
};
constexpr std::array<float, 8> outputSignsLeft {
    1.0f, 1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f, -1.0f
};
constexpr std::array<float, 8> outputSignsRight {
    1.0f, -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f, -1.0f
};

float sanitise(float value, float fallback = 0.0f) noexcept
{
    return std::isfinite(value) ? value : fallback;
}

float millisecondsToSamples(double sampleRate, float milliseconds) noexcept
{
    return static_cast<float>(sampleRate
                              * static_cast<double>(milliseconds) * 0.001);
}

float feedbackForDecay(float delaySeconds, float decaySeconds) noexcept
{
    return std::pow(0.001f,
                    delaySeconds / juce::jmax(0.1f, decaySeconds));
}

float onePoleCoefficient(double sampleRate, float cutoffHz) noexcept
{
    const auto safeCutoff =
        juce::jlimit(20.0, sampleRate * 0.45,
                     static_cast<double>(cutoffHz));
    return static_cast<float>(
        std::exp(-juce::MathConstants<double>::twoPi
                 * safeCutoff / sampleRate));
}

float toneCutoff(float tone,
                 float darkHz,
                 float brightHz) noexcept
{
    const auto unit = 0.5f * (juce::jlimit(-1.0f, 1.0f, tone) + 1.0f);
    return darkHz * std::pow(brightHz / darkHz, unit);
}

float processOnePole(float input,
                     float& state,
                     float coefficient) noexcept
{
    state = input + coefficient * (state - input);
    return state;
}
}

void VocalSpace::MonoDelay::prepare(int maximumSamples)
{
    storage.assign(
        static_cast<size_t>(juce::jmax(4, maximumSamples + 4)), 0.0f);
    writePosition = 0;
}

void VocalSpace::MonoDelay::reset() noexcept
{
    std::fill(storage.begin(), storage.end(), 0.0f);
    writePosition = 0;
}

float VocalSpace::MonoDelay::read(float delaySamples) const noexcept
{
    if (storage.empty())
        return 0.0f;

    const auto capacity = static_cast<int>(storage.size());
    const auto boundedDelay =
        juce::jlimit(1.0f, static_cast<float>(capacity - 2),
                     sanitise(delaySamples, 1.0f));
    auto readPosition = static_cast<float>(writePosition) - boundedDelay;
    while (readPosition < 0.0f)
        readPosition += static_cast<float>(capacity);
    while (readPosition >= static_cast<float>(capacity))
        readPosition -= static_cast<float>(capacity);

    const auto first = static_cast<int>(std::floor(readPosition));
    const auto second = first + 1 == capacity ? 0 : first + 1;
    const auto fraction = readPosition - static_cast<float>(first);
    return storage[static_cast<size_t>(first)]
         + fraction * (storage[static_cast<size_t>(second)]
                       - storage[static_cast<size_t>(first)]);
}

void VocalSpace::MonoDelay::write(float sample) noexcept
{
    if (storage.empty())
        return;

    storage[static_cast<size_t>(writePosition)] = sample;
    if (++writePosition == static_cast<int>(storage.size()))
        writePosition = 0;
}

void VocalSpace::CrossfadedTap::prepare(double sampleRate) noexcept
{
    fadeStep = static_cast<float>(
        1.0 / juce::jmax(1.0, sampleRate * tapCrossfadeMs * 0.001));
    reset();
}

void VocalSpace::CrossfadedTap::reset() noexcept
{
    fromDelaySamples = 0.0f;
    toDelaySamples = 0.0f;
    queuedDelaySamples = 0.0f;
    fade = 1.0f;
    initialised = false;
    hasQueuedTarget = false;
}

float VocalSpace::CrossfadedTap::sampleAt(
    const MonoDelay& delay,
    float delaySamples,
    float directSample) const noexcept
{
    return delaySamples < 0.5f ? directSample : delay.read(delaySamples);
}

float VocalSpace::CrossfadedTap::read(
    const MonoDelay& delay,
    float requestedDelaySamples,
    float modulationSamples,
    float directSample) noexcept
{
    const auto requested =
        juce::jmax(0.0f, sanitise(requestedDelaySamples));

    if (! initialised)
    {
        fromDelaySamples = requested;
        toDelaySamples = requested;
        fade = 1.0f;
        initialised = true;
    }
    else if (fade >= 1.0f
             && std::abs(requested - toDelaySamples) > 0.01f)
    {
        fromDelaySamples = toDelaySamples;
        toDelaySamples = requested;
        fade = 0.0f;
    }
    else if (fade < 1.0f
             && std::abs(requested - toDelaySamples) > 0.01f)
    {
        queuedDelaySamples = requested;
        hasQueuedTarget = true;
    }

    const auto from = sampleAt(delay,
                               fromDelaySamples + modulationSamples,
                               directSample);
    if (fade >= 1.0f)
        return from;

    const auto to = sampleAt(delay,
                             toDelaySamples + modulationSamples,
                             directSample);
    const auto outWeight =
        std::cos(fade * juce::MathConstants<float>::halfPi);
    const auto inWeight =
        std::sin(fade * juce::MathConstants<float>::halfPi);
    const auto output = outWeight * from + inWeight * to;

    fade = juce::jmin(1.0f, fade + fadeStep);
    if (fade >= 1.0f)
    {
        fromDelaySamples = toDelaySamples;
        if (hasQueuedTarget)
        {
            toDelaySamples = queuedDelaySamples;
            fade = 0.0f;
            hasQueuedTarget = false;
        }
    }
    return output;
}

void VocalSpace::AllPass::prepare(double sampleRate, int maximumSamples)
{
    delay.prepare(maximumSamples);
    tap.prepare(sampleRate);
}

void VocalSpace::AllPass::reset() noexcept
{
    delay.reset();
    tap.reset();
}

float VocalSpace::AllPass::process(float input,
                                   float delaySamples,
                                   float modulationSamples,
                                   float feedback) noexcept
{
    const auto delayed =
        tap.read(delay, delaySamples, modulationSamples);
    const auto output = delayed - feedback * input;
    delay.write(input + feedback * output);
    return output;
}

void VocalSpace::Engine::prepare(SpaceMode engineMode,
                                 const ModuleSpec& spec)
{
    mode = engineMode;
    sampleRate = spec.sampleRate;

    const auto maximumPreDelaySamples =
        juce::roundToInt(millisecondsToSamples(sampleRate,
                                               maximumPreDelayMs));
    preDelayLeft.prepare(maximumPreDelaySamples);
    preDelayRight.prepare(maximumPreDelaySamples);
    preDelayTapLeft.prepare(sampleRate);
    preDelayTapRight.prepare(sampleRate);

    float maximumTankSeconds {};
    switch (mode)
    {
        case SpaceMode::room:
            lineCount = 4;
            maximumTankSeconds = 0.10f;
            break;
        case SpaceMode::plate:
            lineCount = 4;
            maximumTankSeconds = 0.15f;
            break;
        case SpaceMode::hall:
            lineCount = 8;
            maximumTankSeconds = 0.50f;
            break;
        case SpaceMode::slap:
            lineCount = 2;
            maximumTankSeconds = 0.30f;
            break;
        case SpaceMode::width:
            lineCount = 2;
            maximumTankSeconds = 0.10f;
            break;
    }

    for (int line = 0; line < lineCount; ++line)
    {
        tank[static_cast<size_t>(line)].prepare(
            juce::roundToInt(sampleRate * maximumTankSeconds));
        tankTaps[static_cast<size_t>(line)].prepare(sampleRate);
    }

    if (mode == SpaceMode::plate)
        for (auto& diffuser : diffusers)
            diffuser.prepare(
                sampleRate,
                juce::roundToInt(millisecondsToSamples(sampleRate, 20.0f)));

    reset();
}

void VocalSpace::Engine::reset() noexcept
{
    preDelayLeft.reset();
    preDelayRight.reset();
    preDelayTapLeft.reset();
    preDelayTapRight.reset();
    for (int line = 0; line < lineCount; ++line)
    {
        tank[static_cast<size_t>(line)].reset();
        tankTaps[static_cast<size_t>(line)].reset();
    }
    if (mode == SpaceMode::plate)
        for (auto& diffuser : diffusers)
            diffuser.reset();
    dampingState.fill(0.0f);
    toneState.fill(0.0f);
    for (size_t phase = 0; phase < modulationPhase.size(); ++phase)
        modulationPhase[phase] = static_cast<float>(phase) * 0.73f;
}

VocalSpace::StereoSample
VocalSpace::Engine::process(StereoSample input,
                            const SpaceSettings& settings) noexcept
{
    switch (mode)
    {
        case SpaceMode::room:
            return processRoom(input, settings);
        case SpaceMode::plate:
            return processPlate(input, settings);
        case SpaceMode::hall:
            return processHall(input, settings);
        case SpaceMode::slap:
            return processSlap(input, settings);
        case SpaceMode::width:
            return processWidth(input, settings);
    }
    return {};
}

VocalSpace::StereoSample
VocalSpace::Engine::processPreDelay(StereoSample input,
                                    float preDelayMs) noexcept
{
    const auto delaySamples =
        millisecondsToSamples(sampleRate, preDelayMs);
    const auto delayedLeft =
        preDelayTapLeft.read(preDelayLeft, delaySamples, 0.0f, input.left);
    const auto delayedRight =
        preDelayTapRight.read(preDelayRight, delaySamples, 0.0f, input.right);
    preDelayLeft.write(input.left);
    preDelayRight.write(input.right);
    return {delayedLeft, delayedRight};
}

VocalSpace::StereoSample
VocalSpace::Engine::processFdn(StereoSample input,
                               const SpaceSettings& settings,
                               const float* delaySeconds,
                               int activeLines,
                               float decayScale,
                               float modulationDepthMs,
                               float inputGain) noexcept
{
    std::array<float, 8> taps {};
    float sum {};
    const auto sizeScale = 0.65f + 0.75f * settings.sizeOrTime;

    for (int line = 0; line < activeLines; ++line)
    {
        auto& phase = modulationPhase[static_cast<size_t>(line)];
        phase += static_cast<float>(
            juce::MathConstants<double>::twoPi
            * (0.11 + 0.017 * static_cast<double>(line)) / sampleRate);
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        const auto modulationSamples =
            millisecondsToSamples(sampleRate, modulationDepthMs)
            * std::sin(phase);
        const auto baseDelaySamples =
            static_cast<float>(sampleRate)
            * delaySeconds[static_cast<size_t>(line)] * sizeScale;
        taps[static_cast<size_t>(line)] =
            tankTaps[static_cast<size_t>(line)].read(
                tank[static_cast<size_t>(line)],
                baseDelaySamples, modulationSamples);
        sum += taps[static_cast<size_t>(line)];
    }

    const auto feedbackCutoff =
        toneCutoff(settings.tone,
                   mode == SpaceMode::plate ? 2500.0f : 1200.0f,
                   mode == SpaceMode::plate ? 18000.0f : 12000.0f);
    const auto damping =
        onePoleCoefficient(sampleRate, feedbackCutoff);
    const auto inverseLines = 1.0f / static_cast<float>(activeLines);
    const auto inputMid = 0.5f * (input.left + input.right);
    const auto inputSide = 0.5f * (input.left - input.right);

    for (int line = 0; line < activeLines; ++line)
    {
        const auto mixed = taps[static_cast<size_t>(line)]
                         - 2.0f * sum * inverseLines;
        const auto damped =
            processOnePole(mixed,
                           dampingState[static_cast<size_t>(line)],
                           damping);
        const auto lineSeconds =
            delaySeconds[static_cast<size_t>(line)] * sizeScale;
        const auto feedback =
            feedbackForDecay(lineSeconds,
                             settings.decaySeconds * decayScale);
        const auto sideSign = line % 2 == 0 ? 1.0f : -1.0f;
        const auto injection =
            inputGain * (inputMid + 0.35f * sideSign * inputSide);
        tank[static_cast<size_t>(line)].write(
            injection + feedback * damped);
    }

    const auto outputScale =
        1.0f / std::sqrt(static_cast<float>(activeLines));
    StereoSample output {};
    for (int line = 0; line < activeLines; ++line)
    {
        output.left += outputSignsLeft[static_cast<size_t>(line)]
                     * taps[static_cast<size_t>(line)] * outputScale;
        output.right += outputSignsRight[static_cast<size_t>(line)]
                      * taps[static_cast<size_t>(line)] * outputScale;
    }

    const auto mid = 0.5f * (output.left + output.right);
    const auto side = 0.5f * (output.left - output.right)
                    * settings.width;
    return {mid + side, mid - side};
}

VocalSpace::StereoSample
VocalSpace::Engine::processRoom(StereoSample input,
                                const SpaceSettings& settings) noexcept
{
    const auto delayed = processPreDelay(input, settings.preDelayMs);
    const auto tankOutput =
        processFdn(delayed, settings, roomDelaySeconds.data(), 4,
                   0.75f, 0.0f, 0.26f);
    return {0.12f * delayed.left + 0.88f * tankOutput.left,
            0.12f * delayed.right + 0.88f * tankOutput.right};
}

VocalSpace::StereoSample
VocalSpace::Engine::processPlate(StereoSample input,
                                 const SpaceSettings& settings) noexcept
{
    const auto delayed = processPreDelay(input, settings.preDelayMs);
    auto diffusedLeft = delayed.left;
    auto diffusedRight = delayed.right;
    const auto sizeScale = 0.75f + 0.5f * settings.sizeOrTime;

    for (int stage = 0; stage < 4; ++stage)
    {
        auto& leftPhase = modulationPhase[static_cast<size_t>(stage)];
        auto& rightPhase = modulationPhase[static_cast<size_t>(stage + 4)];
        leftPhase += static_cast<float>(
            juce::MathConstants<double>::twoPi
            * (0.17 + 0.021 * static_cast<double>(stage)) / sampleRate);
        rightPhase += static_cast<float>(
            juce::MathConstants<double>::twoPi
            * (0.19 + 0.019 * static_cast<double>(stage)) / sampleRate);
        if (leftPhase >= juce::MathConstants<float>::twoPi)
            leftPhase -= juce::MathConstants<float>::twoPi;
        if (rightPhase >= juce::MathConstants<float>::twoPi)
            rightPhase -= juce::MathConstants<float>::twoPi;

        diffusedLeft =
            diffusers[static_cast<size_t>(stage)].process(
                diffusedLeft,
                millisecondsToSamples(
                    sampleRate,
                    plateDiffuserLeftMs[static_cast<size_t>(stage)]
                    * sizeScale),
                millisecondsToSamples(sampleRate, 0.08f)
                    * std::sin(leftPhase),
                0.68f);
        diffusedRight =
            diffusers[static_cast<size_t>(stage + 4)].process(
                diffusedRight,
                millisecondsToSamples(
                    sampleRate,
                    plateDiffuserRightMs[static_cast<size_t>(stage)]
                    * sizeScale),
                millisecondsToSamples(sampleRate, 0.08f)
                    * std::sin(rightPhase),
                0.68f);
    }

    const StereoSample diffused {diffusedLeft, diffusedRight};
    const auto tankOutput =
        processFdn(diffused, settings, plateDelaySeconds.data(), 4,
                   1.0f, 0.18f, 0.22f);
    return {0.18f * diffused.left + 0.82f * tankOutput.left,
            0.18f * diffused.right + 0.82f * tankOutput.right};
}

VocalSpace::StereoSample
VocalSpace::Engine::processHall(StereoSample input,
                                const SpaceSettings& settings) noexcept
{
    const auto delayed = processPreDelay(input, settings.preDelayMs);
    const auto tankOutput =
        processFdn(delayed, settings, hallDelaySeconds.data(), 8,
                   1.5f, 0.42f, 0.18f);
    return {0.06f * delayed.left + 0.94f * tankOutput.left,
            0.06f * delayed.right + 0.94f * tankOutput.right};
}

VocalSpace::StereoSample
VocalSpace::Engine::processSlap(StereoSample input,
                                const SpaceSettings& settings) noexcept
{
    const auto timeSamples =
        millisecondsToSamples(sampleRate, settings.preDelayMs);
    const auto spreadMs = 9.0f * settings.width;
    const auto leftTap =
        tankTaps[0].read(tank[0], timeSamples);
    const auto rightTap =
        tankTaps[1].read(
            tank[1],
            timeSamples + millisecondsToSamples(sampleRate, spreadMs));

    const auto cutoff =
        toneCutoff(settings.tone, 900.0f, 14000.0f);
    const auto damping =
        onePoleCoefficient(sampleRate, cutoff);
    const auto filteredLeft =
        processOnePole(leftTap, toneState[0], damping);
    const auto filteredRight =
        processOnePole(rightTap, toneState[1], damping);
    const auto feedback =
        juce::jlimit(0.0f, 0.92f, settings.feedback);
    tank[0].write(input.left
                  + feedback
                    * (0.88f * filteredLeft + 0.12f * filteredRight));
    tank[1].write(input.right
                  + feedback
                    * (0.88f * filteredRight + 0.12f * filteredLeft));
    return {filteredLeft, filteredRight};
}

VocalSpace::StereoSample
VocalSpace::Engine::processWidth(StereoSample input,
                                 const SpaceSettings& settings) noexcept
{
    const auto mid = 0.5f * (input.left + input.right);
    const auto spreadMs = 5.0f + 30.0f * settings.sizeOrTime;
    const auto first =
        tankTaps[0].read(
            tank[0],
            millisecondsToSamples(sampleRate, spreadMs * 0.55f));
    const auto second =
        tankTaps[1].read(
            tank[1],
            millisecondsToSamples(sampleRate, spreadMs));
    tank[0].write(mid);
    tank[1].write(mid);

    const auto cutoff =
        toneCutoff(settings.tone, 900.0f, 18000.0f);
    const auto coefficient =
        onePoleCoefficient(sampleRate, cutoff);
    const auto tonedFirst =
        processOnePole(first, toneState[0], coefficient);
    const auto tonedSecond =
        processOnePole(second, toneState[1], coefficient);
    const auto side =
        0.5f * (tonedFirst - tonedSecond) * settings.width;

    if (settings.monoSafety)
        return {side, -side};

    return {tonedFirst * settings.width,
            tonedSecond * settings.width};
}

void VocalSpace::prepare(const ModuleSpec& spec)
{
    moduleSpec.sampleRate = juce::jmax(1.0, spec.sampleRate);
    moduleSpec.maximumBlockSize = juce::jmax(1, spec.maximumBlockSize);
    moduleSpec.channels = juce::jlimit(1, 2, spec.channels);

    for (const auto mode : {SpaceMode::room, SpaceMode::plate,
                            SpaceMode::hall, SpaceMode::slap,
                            SpaceMode::width})
        engines[modeIndex(mode)].prepare(mode, moduleSpec);

    parameterCoefficient =
        smoothingCoefficient(moduleSpec.sampleRate, parameterSmoothingMs);
    duckAttackCoefficient =
        smoothingCoefficient(moduleSpec.sampleRate, duckAttackMs);
    duckReleaseCoefficient =
        smoothingCoefficient(moduleSpec.sampleRate, duckReleaseMs);
    modeFadeStep = static_cast<float>(
        1.0 / juce::jmax(1.0,
                         moduleSpec.sampleRate * modeCrossfadeMs * 0.001));
    prepared = true;
    reset();
}

void VocalSpace::reset() noexcept
{
    for (auto& engine : engines)
        engine.reset();

    currentSettings = targetSettings;
    currentMode = sanitiseMode(targetSettings.mode);
    nextMode = currentMode;
    modeFade = 1.0f;
    duckEnvelope = 0.0f;
    hasProcessed = false;
}

void VocalSpace::setTargetSettings(const SpaceSettings& settings) noexcept
{
    auto safe = settings;
    safe.mode = sanitiseMode(safe.mode);
    safe.amount = juce::jlimit(0.0f, 1.0f, sanitise(safe.amount));
    safe.preDelayMs =
        juce::jlimit(0.0f, maximumPreDelayMs,
                     sanitise(safe.preDelayMs, 28.0f));
    safe.sizeOrTime =
        juce::jlimit(0.0f, 1.0f, sanitise(safe.sizeOrTime, 0.62f));
    safe.decaySeconds =
        juce::jlimit(0.1f, 8.0f, sanitise(safe.decaySeconds, 1.6f));
    safe.tone = juce::jlimit(-1.0f, 1.0f, sanitise(safe.tone, 0.12f));
    safe.width = juce::jlimit(0.0f, 2.0f, sanitise(safe.width, 1.0f));
    safe.ducking =
        juce::jlimit(0.0f, 1.0f, sanitise(safe.ducking, 0.42f));
    safe.feedback =
        juce::jlimit(0.0f, 0.92f, sanitise(safe.feedback, 0.2f));
    targetSettings = safe;

    if (! hasProcessed)
    {
        currentSettings = safe;
        currentMode = safe.mode;
        nextMode = safe.mode;
        modeFade = 1.0f;
    }
}

void VocalSpace::beginModeTransitionIfNeeded() noexcept
{
    if (modeFade < 1.0f || targetSettings.mode == currentMode)
        return;

    nextMode = targetSettings.mode;
    engineFor(nextMode).reset();
    modeFade = 0.0f;
}

void VocalSpace::process(
    juce::AudioBuffer<float>& audio,
    const juce::AudioBuffer<float>& drySidechain) noexcept
{
    if (! prepared || audio.getNumSamples() <= 0
        || audio.getNumChannels() <= 0)
        return;

    hasProcessed = true;
    beginModeTransitionIfNeeded();

    const auto channels =
        juce::jmin(audio.getNumChannels(), moduleSpec.channels);
    const auto samples =
        juce::jmin(audio.getNumSamples(), moduleSpec.maximumBlockSize);

    for (int sample = 0; sample < samples; ++sample)
    {
        currentSettings.amount =
            advance(currentSettings.amount,
                    targetSettings.amount,
                    parameterCoefficient);
        currentSettings.preDelayMs = targetSettings.preDelayMs;
        currentSettings.sizeOrTime = targetSettings.sizeOrTime;
        currentSettings.decaySeconds =
            advance(currentSettings.decaySeconds,
                    targetSettings.decaySeconds,
                    parameterCoefficient);
        currentSettings.tone =
            advance(currentSettings.tone,
                    targetSettings.tone,
                    parameterCoefficient);
        currentSettings.width =
            advance(currentSettings.width,
                    targetSettings.width,
                    parameterCoefficient);
        currentSettings.ducking =
            advance(currentSettings.ducking,
                    targetSettings.ducking,
                    parameterCoefficient);
        currentSettings.feedback =
            advance(currentSettings.feedback,
                    targetSettings.feedback,
                    parameterCoefficient);
        currentSettings.monoSafety = targetSettings.monoSafety;

        const auto left = audio.getSample(0, sample);
        const auto right = channels > 1
                             ? audio.getSample(1, sample)
                             : left;
        const StereoSample input {left, right};
        const auto currentWet =
            engineFor(currentMode).process(input, currentSettings);
        auto wet = currentWet;

        if (modeFade < 1.0f)
        {
            const auto nextWet =
                engineFor(nextMode).process(input, currentSettings);
            const auto inWeight =
                std::sin(modeFade
                         * juce::MathConstants<float>::halfPi);
            const auto outWeight =
                std::cos(modeFade
                         * juce::MathConstants<float>::halfPi);
            wet.left = outWeight * currentWet.left
                     + inWeight * nextWet.left;
            wet.right = outWeight * currentWet.right
                      + inWeight * nextWet.right;
            modeFade = juce::jmin(1.0f, modeFade + modeFadeStep);
            if (modeFade >= 1.0f)
            {
                currentMode = nextMode;
                beginModeTransitionIfNeeded();
            }
        }

        float sidechainLevel {};
        if (sample < drySidechain.getNumSamples())
            for (int channel = 0;
                 channel < juce::jmin(drySidechain.getNumChannels(), 2);
                 ++channel)
                sidechainLevel =
                    juce::jmax(
                        sidechainLevel,
                        std::abs(
                            drySidechain.getSample(channel, sample)));

        const auto duckCoefficient =
            sidechainLevel > duckEnvelope
                ? duckAttackCoefficient
                : duckReleaseCoefficient;
        duckEnvelope =
            sidechainLevel
            + duckCoefficient * (duckEnvelope - sidechainLevel);
        const auto duckGain =
            1.0f / (1.0f + 8.0f
                    * currentSettings.ducking * duckEnvelope);
        const auto wetGain =
            currentSettings.amount * duckGain;

        const auto outputLeft = left + wetGain * wet.left;
        audio.setSample(0, sample,
                        std::isfinite(outputLeft)
                            ? outputLeft : left);
        if (channels > 1)
        {
            const auto outputRight = right + wetGain * wet.right;
            audio.setSample(1, sample,
                            std::isfinite(outputRight)
                                ? outputRight : right);
        }
    }
}

double VocalSpace::tailSeconds() const noexcept
{
    switch (targetSettings.mode)
    {
        case SpaceMode::room:
            return static_cast<double>(targetSettings.preDelayMs) * 0.001
                 + 0.75 * static_cast<double>(targetSettings.decaySeconds);
        case SpaceMode::plate:
            return static_cast<double>(targetSettings.preDelayMs) * 0.001
                 + static_cast<double>(targetSettings.decaySeconds);
        case SpaceMode::hall:
            return static_cast<double>(targetSettings.preDelayMs) * 0.001
                 + 1.5 * static_cast<double>(targetSettings.decaySeconds);
        case SpaceMode::slap:
        {
            const auto repeatSeconds =
                static_cast<double>(targetSettings.preDelayMs) * 0.001;
            const auto spreadSeconds =
                0.009 * static_cast<double>(targetSettings.width);
            if (targetSettings.feedback <= 0.0f)
                return repeatSeconds + spreadSeconds;
            const auto repeats =
                std::log(0.001)
                / std::log(static_cast<double>(
                      targetSettings.feedback));
            return (repeatSeconds + spreadSeconds)
                 * juce::jmax(1.0, repeats);
        }
        case SpaceMode::width:
            return 0.005
                 + 0.030 * static_cast<double>(
                               targetSettings.sizeOrTime);
    }
    return 0.0;
}

size_t VocalSpace::modeIndex(SpaceMode mode) noexcept
{
    return static_cast<size_t>(sanitiseMode(mode));
}

SpaceMode VocalSpace::sanitiseMode(SpaceMode mode) noexcept
{
    switch (mode)
    {
        case SpaceMode::room:
        case SpaceMode::plate:
        case SpaceMode::hall:
        case SpaceMode::slap:
        case SpaceMode::width:
            return mode;
    }
    return SpaceMode::plate;
}

float VocalSpace::advance(float current,
                          float target,
                          float coefficient) noexcept
{
    const auto next = target + coefficient * (current - target);
    return std::abs(next - target) < 1.0e-7f ? target : next;
}

VocalSpace::Engine& VocalSpace::engineFor(SpaceMode mode) noexcept
{
    return engines[modeIndex(mode)];
}
}
