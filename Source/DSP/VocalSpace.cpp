#include "VocalSpace.h"

#include <algorithm>
#include <cmath>

namespace Voxline::Dsp
{
namespace
{
constexpr float parameterSmoothingMs = 10.0f;
constexpr float modeCrossfadeMs = 30.0f;
constexpr float duckAttackMs = 5.0f;
constexpr float duckReleaseMs = 180.0f;
constexpr float maximumPreDelayMs = 250.0f;

float sanitise(float value, float fallback = 0.0f) noexcept
{
    return std::isfinite(value) ? value : fallback;
}

int millisecondsToSamples(double sampleRate, float milliseconds) noexcept
{
    return juce::roundToInt(sampleRate
                            * static_cast<double>(milliseconds) * 0.001);
}

float feedbackForDecay(float delaySeconds, float decaySeconds) noexcept
{
    return std::pow(0.001f,
                    delaySeconds / juce::jmax(0.1f, decaySeconds));
}
}

void VocalSpace::DelayLine::prepare(int samples)
{
    const auto capacity = juce::jmax(2, samples + 2);
    left.assign(static_cast<size_t>(capacity), 0.0f);
    right.assign(static_cast<size_t>(capacity), 0.0f);
    writePosition = 0;
}

void VocalSpace::DelayLine::reset() noexcept
{
    std::fill(left.begin(), left.end(), 0.0f);
    std::fill(right.begin(), right.end(), 0.0f);
    writePosition = 0;
}

VocalSpace::StereoSample
VocalSpace::DelayLine::read(int delaySamples) const noexcept
{
    if (left.empty())
        return {};

    const auto capacity = static_cast<int>(left.size());
    auto readPosition = writePosition
                      - juce::jlimit(1, capacity - 1, delaySamples);
    if (readPosition < 0)
        readPosition += capacity;
    return {left[static_cast<size_t>(readPosition)],
            right[static_cast<size_t>(readPosition)]};
}

void VocalSpace::DelayLine::write(StereoSample sample) noexcept
{
    if (left.empty())
        return;

    left[static_cast<size_t>(writePosition)] = sample.left;
    right[static_cast<size_t>(writePosition)] = sample.right;
    if (++writePosition == static_cast<int>(left.size()))
        writePosition = 0;
}

void VocalSpace::Engine::prepare(SpaceMode engineMode,
                                 const ModuleSpec& spec)
{
    mode = engineMode;
    sampleRate = spec.sampleRate;
    preDelay.prepare(millisecondsToSamples(sampleRate, maximumPreDelayMs));

    float maximumLineSeconds {};
    switch (mode)
    {
        case SpaceMode::room:
            lineCount = 4;
            maximumLineSeconds = 0.10f;
            break;
        case SpaceMode::plate:
            lineCount = 6;
            maximumLineSeconds = 0.20f;
            break;
        case SpaceMode::hall:
            lineCount = 8;
            maximumLineSeconds = 0.50f;
            break;
        case SpaceMode::slap:
            lineCount = 2;
            maximumLineSeconds = 2.10f;
            break;
        case SpaceMode::width:
            lineCount = 2;
            maximumLineSeconds = 0.10f;
            break;
    }

    for (int line = 0; line < lineCount; ++line)
        lines[static_cast<size_t>(line)].prepare(
            juce::roundToInt(sampleRate * maximumLineSeconds));

    reset();
}

void VocalSpace::Engine::reset() noexcept
{
    preDelay.reset();
    for (int line = 0; line < lineCount; ++line)
        lines[static_cast<size_t>(line)].reset();
    diffusion.fill(0.0f);
    toneState.fill(0.0f);
    modulationPhase = 0.0f;
}

VocalSpace::StereoSample
VocalSpace::Engine::process(StereoSample input,
                            const SpaceSettings& settings) noexcept
{
    switch (mode)
    {
        case SpaceMode::room:
        case SpaceMode::plate:
        case SpaceMode::hall:
            return processReverb(input, settings);
        case SpaceMode::slap:
            return processSlap(input, settings);
        case SpaceMode::width:
            return processWidth(input, settings);
    }
    return {};
}

VocalSpace::StereoSample
VocalSpace::Engine::processReverb(StereoSample input,
                                  const SpaceSettings& settings) noexcept
{
    const auto preDelaySamples =
        millisecondsToSamples(sampleRate, settings.preDelayMs);
    const auto delayed = preDelaySamples == 0
                           ? input
                           : preDelay.read(preDelaySamples);
    preDelay.write(input);

    static constexpr std::array roomDelays {
        0.0113f, 0.0179f, 0.0297f, 0.0371f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    static constexpr std::array plateDelays {
        0.0131f, 0.0197f, 0.0277f, 0.0391f,
        0.0533f, 0.0719f, 0.0f, 0.0f
    };
    static constexpr std::array hallDelays {
        0.0371f, 0.0533f, 0.0719f, 0.0899f,
        0.113f, 0.149f, 0.193f, 0.241f
    };

    const auto& baseDelays = mode == SpaceMode::room
                               ? roomDelays
                               : (mode == SpaceMode::plate
                                    ? plateDelays
                                    : hallDelays);
    const auto sizeScale = 0.6f + settings.sizeOrTime * 0.8f;
    std::array<StereoSample, 8> taps {};
    StereoSample sum {};

    modulationPhase += static_cast<float>(
        juce::MathConstants<double>::twoPi
        * (mode == SpaceMode::hall ? 0.23 : 0.11) / sampleRate);
    if (modulationPhase > juce::MathConstants<float>::twoPi)
        modulationPhase -= juce::MathConstants<float>::twoPi;

    for (int line = 0; line < lineCount; ++line)
    {
        auto delaySeconds = baseDelays[static_cast<size_t>(line)] * sizeScale;
        if (mode == SpaceMode::hall)
            delaySeconds += 0.00007f
                          * static_cast<float>(line + 1)
                          * std::sin(modulationPhase
                                     + static_cast<float>(line) * 0.73f);
        const auto delaySamples = juce::jmax(
            1, juce::roundToInt(delaySeconds * sampleRate));
        taps[static_cast<size_t>(line)] =
            lines[static_cast<size_t>(line)].read(delaySamples);
        sum.left += taps[static_cast<size_t>(line)].left;
        sum.right += taps[static_cast<size_t>(line)].right;
    }

    const auto normaliser = 1.0f / static_cast<float>(lineCount);
    for (int line = 0; line < lineCount; ++line)
    {
        const auto delaySeconds =
            baseDelays[static_cast<size_t>(line)] * sizeScale;
        const auto feedback =
            feedbackForDecay(delaySeconds,
                             settings.decaySeconds
                             * (mode == SpaceMode::room ? 0.75f
                                : mode == SpaceMode::hall ? 1.5f : 1.0f));
        const auto mixedLeft =
            (sum.left - 2.0f * taps[static_cast<size_t>(line)].left)
            * normaliser;
        const auto mixedRight =
            (sum.right - 2.0f * taps[static_cast<size_t>(line)].right)
            * normaliser;
        const auto cross = line % 2 == 0 ? 0.13f : -0.13f;
        lines[static_cast<size_t>(line)].write({
            delayed.left * (0.22f + 0.03f * static_cast<float>(line))
                + feedback * (mixedLeft + cross * mixedRight),
            delayed.right * (0.22f + 0.03f * static_cast<float>(line))
                + feedback * (mixedRight + cross * mixedLeft)
        });
    }

    StereoSample wet {
        delayed.left * 0.18f + sum.left * normaliser,
        delayed.right * 0.18f + sum.right * normaliser
    };

    if (mode == SpaceMode::plate || mode == SpaceMode::hall)
    {
        const auto diffusionFeedback =
            mode == SpaceMode::plate ? 0.72f : 0.58f;
        diffusion[0] = delayed.left
                     + diffusionFeedback * diffusion[0];
        diffusion[1] = delayed.right
                     + diffusionFeedback * diffusion[1];
        wet.left += (mode == SpaceMode::plate ? 0.14f : 0.08f)
                  * diffusion[0];
        wet.right += (mode == SpaceMode::plate ? 0.14f : 0.08f)
                   * diffusion[1];
    }

    if (mode == SpaceMode::room)
    {
        const auto brightness =
            juce::jlimit(0.55f, 1.15f, 0.82f + settings.tone * 0.3f);
        wet.left *= brightness;
        wet.right *= brightness;
    }
    else
    {
        const auto damping = juce::jlimit(
            0.05f, 0.92f, 0.58f - settings.tone * 0.32f);
        toneState[0] += (1.0f - damping) * (wet.left - toneState[0]);
        toneState[1] += (1.0f - damping) * (wet.right - toneState[1]);
        wet.left = toneState[0];
        wet.right = toneState[1];
    }

    const auto sideScale = juce::jlimit(0.0f, 2.0f, settings.width);
    const auto mid = 0.5f * (wet.left + wet.right);
    const auto side = 0.5f * (wet.left - wet.right) * sideScale;
    return {mid + side, mid - side};
}

VocalSpace::StereoSample
VocalSpace::Engine::processSlap(StereoSample input,
                                const SpaceSettings& settings) noexcept
{
    const auto delaySamples = juce::jmax(
        1, millisecondsToSamples(sampleRate, settings.preDelayMs));
    const auto delayed = lines[0].read(delaySamples);
    const auto feedback = juce::jlimit(0.0f, 0.92f, settings.feedback);
    lines[0].write({input.left + feedback * delayed.left,
                    input.right + feedback * delayed.right});

    const auto damping = juce::jlimit(
        0.05f, 0.92f, 0.58f - settings.tone * 0.32f);
    toneState[0] += (1.0f - damping) * (delayed.left - toneState[0]);
    toneState[1] += (1.0f - damping) * (delayed.right - toneState[1]);

    const auto mid = 0.5f * (toneState[0] + toneState[1]);
    const auto side = 0.5f * (toneState[0] - toneState[1])
                    * juce::jlimit(0.0f, 2.0f, settings.width);
    return {mid + side, mid - side};
}

VocalSpace::StereoSample
VocalSpace::Engine::processWidth(StereoSample input,
                                 const SpaceSettings& settings) noexcept
{
    const auto mid = 0.5f * (input.left + input.right);
    const auto maximumSpreadMs = 5.0f + settings.sizeOrTime * 30.0f;
    const auto shortDelay =
        millisecondsToSamples(sampleRate, maximumSpreadMs * 0.55f);
    const auto longDelay =
        millisecondsToSamples(sampleRate, maximumSpreadMs);
    const auto first = lines[0].read(shortDelay);
    const auto second = lines[1].read(longDelay);
    lines[0].write({mid, mid});
    lines[1].write({mid, mid});

    const auto spread = 0.5f * (first.left - second.left)
                      * juce::jlimit(0.0f, 2.0f, settings.width);
    if (settings.monoSafety)
        return {spread, -spread};

    return {first.left * settings.width,
            second.right * settings.width};
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
    prepared = true;
    reset();
}

void VocalSpace::reset() noexcept
{
    for (auto& engine : engines)
        engine.reset();

    currentSettings = targetSettings;
    currentMode = targetSettings.mode;
    nextMode = currentMode;
    modeFade = 1.0f;
    duckEnvelope = 0.0f;
    hasProcessed = false;
}

void VocalSpace::setTargetSettings(const SpaceSettings& settings) noexcept
{
    auto safe = settings;
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

void VocalSpace::process(
    juce::AudioBuffer<float>& audio,
    const juce::AudioBuffer<float>& drySidechain) noexcept
{
    if (! prepared || audio.getNumSamples() <= 0
        || audio.getNumChannels() <= 0)
        return;

    hasProcessed = true;
    if (targetSettings.mode != nextMode)
    {
        if (modeFade >= 0.5f)
            currentMode = nextMode;
        nextMode = targetSettings.mode;
        modeFade = currentMode == nextMode ? 1.0f : 0.0f;
    }

    const auto channels = juce::jmin(audio.getNumChannels(),
                                    moduleSpec.channels);
    const auto samples = juce::jmin(audio.getNumSamples(),
                                   moduleSpec.maximumBlockSize);
    const auto fadeStep = static_cast<float>(
        1.0 / juce::jmax(1.0,
                         moduleSpec.sampleRate
                         * modeCrossfadeMs * 0.001));

    for (int sample = 0; sample < samples; ++sample)
    {
        currentSettings.amount =
            advance(currentSettings.amount,
                    targetSettings.amount,
                    parameterCoefficient);
        currentSettings.preDelayMs =
            advance(currentSettings.preDelayMs,
                    targetSettings.preDelayMs,
                    parameterCoefficient);
        currentSettings.sizeOrTime =
            advance(currentSettings.sizeOrTime,
                    targetSettings.sizeOrTime,
                    parameterCoefficient);
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
        auto currentWet = engineFor(currentMode).process(
            input, currentSettings);
        auto wet = currentWet;

        if (modeFade < 1.0f)
        {
            const auto nextWet = engineFor(nextMode).process(
                input, currentSettings);
            const auto equalPowerIn =
                std::sin(modeFade
                         * juce::MathConstants<float>::halfPi);
            const auto equalPowerOut =
                std::cos(modeFade
                         * juce::MathConstants<float>::halfPi);
            wet.left = equalPowerOut * currentWet.left
                     + equalPowerIn * nextWet.left;
            wet.right = equalPowerOut * currentWet.right
                      + equalPowerIn * nextWet.right;
            modeFade = juce::jmin(1.0f, modeFade + fadeStep);
            if (modeFade >= 1.0f)
                currentMode = nextMode;
        }

        float sidechainLevel {};
        if (sample < drySidechain.getNumSamples())
            for (int channel = 0;
                 channel < juce::jmin(drySidechain.getNumChannels(), 2);
                 ++channel)
                sidechainLevel = juce::jmax(
                    sidechainLevel,
                    std::abs(drySidechain.getSample(channel, sample)));

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
        const auto wetGain = currentSettings.amount * duckGain;

        audio.setSample(0, sample,
                        std::isfinite(left + wetGain * wet.left)
                            ? left + wetGain * wet.left : left);
        if (channels > 1)
            audio.setSample(1, sample,
                            std::isfinite(right + wetGain * wet.right)
                                ? right + wetGain * wet.right : right);
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
            if (targetSettings.feedback <= 0.0f)
                return repeatSeconds;
            const auto repeats = std::log(0.001)
                               / std::log(
                                     static_cast<double>(
                                         targetSettings.feedback));
            return repeatSeconds * juce::jmax(1.0, repeats);
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
    return static_cast<size_t>(mode);
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
