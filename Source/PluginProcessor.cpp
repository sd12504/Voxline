#include "PluginProcessor.h"
#include "PluginEditor.h"

// VOXLINE DSP signal flow
// 1. Input Gain: smoothed gain staging before detection and processing.
// 2. Clean Mode: optional rumble HPF gate/cleanup before tone shaping.
// 3. Vocal EQ: dedicated EQ chain HPF -> LOW -> MUD -> PRES -> AIR -> LPF.
//    The eqEnabled parameter only bypasses this dedicated EQ stage; legacy tone
//    smooth/body/clarity/air processing and SPACE remain independent.
// 4. Compressor: level detector drives soft-knee gain reduction with variable timing.
// 5. Drive/Saturation: conservative asymmetric saturation with pre/de-emphasis.
// 6. SPACE: optional vocal ambience after dynamics and saturation.
// 7. Output Gain, safety soft clip, listen/bypass crossfade, then meters.

namespace
{
float percentToUnit(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value * 0.01f);
}

juce::AudioParameterFloatAttributes makeDbAttributes()
{
    juce::AudioParameterFloatAttributes attributes;
    return attributes.withStringFromValueFunction([](float value, int)
                                                  { return juce::String(value, 1) + " dB"; });
}

juce::AudioParameterFloatAttributes makePercentAttributes()
{
    juce::AudioParameterFloatAttributes attributes;
    return attributes.withStringFromValueFunction([](float value, int)
                                                  { return juce::String(value, 0) + " %"; });
}
} // namespace

VoxlineAudioProcessor::VoxlineAudioProcessor()
    : AudioProcessor(BusesProperties()
    #if ! JucePlugin_IsMidiEffect
     #if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
     #endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
      ),
      apvts(*this, nullptr, "VOXLINEState", createParameterLayout())
{
}

void VoxlineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax(1.0, sampleRate);

    inputGainSmoothed.reset(currentSampleRate, 0.02);
    outputGainSmoothed.reset(currentSampleRate, 0.02);
    bypassSmoothed.reset(currentSampleRate, 0.005);
    bypassSmoothed.setCurrentAndTargetValue(0.0f);
    inputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::inputGain)->load()));
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::outputGain)->load()));

    for (auto& channelFilters : hpfFilters)
        for (auto& filter : channelFilters)
            filter.reset();
    for (auto& channelFilters : lpfFilters)
        for (auto& filter : channelFilters)
            filter.reset();
    for (auto* filters : { &bodyFilters, &mudFilters, &clarityFilters, &airFilters, &smoothFilters, &lowFilters })
        for (auto& filter : *filters)
            filter.reset();

    dryBuffer.setSize(juce::jmax(1, getTotalNumInputChannels()), juce::jmax(1, samplesPerBlock), false, false, true);
    maxSpaceDelaySamples = juce::jmax(1, static_cast<int>(std::ceil(currentSampleRate * 3.0)));
    spaceBuffer.setSize(juce::jmax(2, getTotalNumOutputChannels()), maxSpaceDelaySamples, false, false, true);
    spaceBuffer.clear();
    spaceWritePos = 0;
    compressorEnvelope = 1.0f;
    cleanModeXPrev[0] = cleanModeXPrev[1] = 0.0f;
    cleanModeYPrev[0] = cleanModeYPrev[1] = 0.0f;
    drivePreEmphasisState[0] = drivePreEmphasisState[1] = 0.0f;
    driveDeEmphasisState[0] = driveDeEmphasisState[1] = 0.0f;
    deEssLowpassState[0] = deEssLowpassState[1] = 0.0f;
    spaceDuckEnvelope = 0.0f;
    updateToneFilters();
}

void VoxlineAudioProcessor::releaseResources()
{
}

#if ! JucePlugin_IsMidiEffect
bool VoxlineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
   #if JucePlugin_IsSynth
    juce::ignoreUnused(layouts);
    return true;
   #else
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
    {
        return false;
    }

   #if ! JucePlugin_IsSynth
    if (mainOutput != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}
#endif

void VoxlineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    const auto numInputChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numInputChannels <= 0 || numOutputChannels <= 0 || numSamples <= 0)
        return;

    dryBuffer.setSize(numInputChannels, numSamples, false, false, true);
    for (auto channel = 0; channel < numInputChannels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

    // Calculate input meters from dry buffer
    float inPeak = 0.0f, inRms = 0.0f;
    for (auto channel = 0; channel < numInputChannels; ++channel)
    {
        auto* data = dryBuffer.getReadPointer(channel);
        for (auto i = 0; i < numSamples; ++i)
        {
            const auto s = std::abs(data[i]);
            inPeak = juce::jmax(inPeak, s);
            inRms += s * s;
        }
    }
    inRms = std::sqrt(inRms / static_cast<float>(numSamples * numInputChannels));

    // Smooth and store input meters
    const auto meterAttack = std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.008f));
    const auto meterRelease = std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.400f));
    const auto inPeakDb = juce::Decibels::gainToDecibels(inPeak, -60.0f);
    const auto inRmsDb = juce::Decibels::gainToDecibels(inRms, -60.0f);

    auto targetInPeak = juce::jlimit(0.0f, 1.0f, (inPeakDb + 60.0f) / 60.0f);
    auto targetInRms = juce::jlimit(0.0f, 1.0f, (inRmsDb + 60.0f) / 60.0f);
    auto prevInPeak = inputPeak.load();
    auto prevInRms = inputRms.load();
    auto ipCoeff = targetInPeak > prevInPeak ? meterAttack : meterRelease;
    auto irCoeff = targetInRms > prevInRms ? meterAttack : meterRelease;
    inputPeak.store(prevInPeak + ipCoeff * (targetInPeak - prevInPeak));
    inputRms.store(prevInRms + irCoeff * (targetInRms - prevInRms));

    if (apvts.getRawParameterValue(VoxlineParameterIDs::bypass)->load() >= 0.5f)
    {
        bypassSmoothed.setTargetValue(1.0f);
        auto prevOutPeak = outputPeak.load();
        auto prevOutRms = outputRms.load();
        auto opCoeff = targetInPeak > prevOutPeak ? meterAttack : meterRelease;
        auto orCoeff = targetInRms > prevOutRms ? meterAttack : meterRelease;
        outputPeak.store(prevOutPeak + opCoeff * (targetInPeak - prevOutPeak));
        outputRms.store(prevOutRms + orCoeff * (targetInRms - prevOutRms));
        gainReduction.store(0.0f);
    }
    else
    {
        bypassSmoothed.setTargetValue(0.0f);
    }

    const auto polishRaw = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::polish)->load());
    const auto polishScale = juce::jmap(polishRaw, 0.0f, 1.0f, 0.35f, 1.35f);
    const auto compRaw = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::comp)->load());
    const auto driveRaw = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::drive)->load());

    const auto compAmount = juce::jlimit(0.0f, 1.0f, compRaw * polishScale);
    const auto driveAmount = juce::jlimit(0.0f, 1.0f, driveRaw * polishScale);
    const auto autoGainEnabled = apvts.getRawParameterValue(VoxlineParameterIDs::autoGain)->load() >= 0.5f;
    const auto listenEnabled = apvts.getRawParameterValue(VoxlineParameterIDs::listen)->load() >= 0.5f;
    const auto cleanModeOn = apvts.getRawParameterValue(VoxlineParameterIDs::cleanMode)->load() >= 0.5f;

    const auto spaceAmount = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::spaceAmount)->load());
    const auto spaceType = static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::spaceType)->load());
    const auto spaceTimeMs = apvts.getRawParameterValue(VoxlineParameterIDs::spaceTime)->load();
    const auto spacePreDelayMs = apvts.getRawParameterValue(VoxlineParameterIDs::spacePreDelay)->load();
    const auto spaceWidth = apvts.getRawParameterValue(VoxlineParameterIDs::spaceWidth)->load() * 0.01f;
    const auto spaceTone = apvts.getRawParameterValue(VoxlineParameterIDs::spaceTone)->load() * 0.01f;
    const auto spaceDecay = apvts.getRawParameterValue(VoxlineParameterIDs::spaceDecay)->load();
    const auto spaceDucking = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::spaceDucking)->load());

    inputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::inputGain)->load()));

    auto outputGainDb = apvts.getRawParameterValue(VoxlineParameterIDs::outputGain)->load();
    if (autoGainEnabled)
    {
        const auto eqOn = apvts.getRawParameterValue(VoxlineParameterIDs::eqEnabled)->load() > 0.5f;
        const auto lowBoostDb = eqOn ? juce::jmax(0.0f, apvts.getRawParameterValue(VoxlineParameterIDs::body)->load() * polishScale) : 0.0f;
        const auto mudBoostDb = eqOn ? juce::jmax(0.0f, apvts.getRawParameterValue(VoxlineParameterIDs::mudGain)->load() * polishScale) : 0.0f;
        const auto presBoostDb = eqOn ? juce::jmax(0.0f, apvts.getRawParameterValue(VoxlineParameterIDs::clarity)->load() * polishScale) : 0.0f;
        const auto airBoostDb = eqOn ? juce::jmax(0.0f, apvts.getRawParameterValue(VoxlineParameterIDs::air)->load() * polishScale) : 0.0f;
        const auto eqBoostCompDb = juce::jlimit(0.0f, 2.0f,
                                               (lowBoostDb * 0.10f) + (mudBoostDb * 0.08f)
                                               + (presBoostDb * 0.12f) + (airBoostDb * 0.10f));
        outputGainDb += compAmount * 2.3f - driveAmount * 0.9f - eqBoostCompDb;
    }
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));

    updateToneFilters();
    updateEQFilters();

    const auto driveCharacter = static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::driveCharacter)->load());
    const auto driveTone = apvts.getRawParameterValue(VoxlineParameterIDs::driveTone)->load() * 0.01f;
    const auto driveMixControl = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::driveMix)->load());
    const auto characterGainDb = driveCharacter == 0 ? 7.0f : (driveCharacter == 1 ? 9.0f : 12.0f);
    const auto drivePreGain = juce::Decibels::decibelsToGain(driveAmount * characterGainDb);
    const auto driveNormalizer = juce::jmax(0.35f, std::tanh(drivePreGain));
    const auto driveToneAmount = driveAmount * 0.35f;
    const auto driveDeEmphasisAmount = driveAmount * 0.22f;
    const auto driveToneFrequency = juce::jmap(driveTone, -1.0f, 1.0f, 1400.0f, 4800.0f);
    const auto driveToneCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * driveToneFrequency
                                         / static_cast<float>(currentSampleRate));
    const auto wetMix = juce::jlimit(0.0f, 1.0f, 0.35f + polishScale * 0.30f);
    const auto deEssAmount = juce::jlimit(0.0f, 1.0f,
        percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::smooth)->load()) * polishScale);
    const auto deEssFreq = apvts.getRawParameterValue(VoxlineParameterIDs::deEssFreq)->load();
    const auto deEssThreshold = apvts.getRawParameterValue(VoxlineParameterIDs::deEssThreshold)->load();
    const auto deEssRange = apvts.getRawParameterValue(VoxlineParameterIDs::deEssRange)->load();
    const auto deEssWideband = apvts.getRawParameterValue(VoxlineParameterIDs::deEssMode)->load() >= 0.5f;
    const auto vocalEqEnabled = apvts.getRawParameterValue(VoxlineParameterIDs::eqEnabled)->load() >= 0.5f;
    const auto activeHpfStages = juce::jlimit(1, 4,
        static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::hpfSlope)->load()) + 1);
    const auto activeLpfStages = juce::jlimit(1, 2,
        static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::lpfSlope)->load()) + 1);
    const auto deEssCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * deEssFreq
                                     / static_cast<float>(currentSampleRate));
    const auto cleanModeCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * 80.0f
                                         / static_cast<float>(currentSampleRate));
    const auto duckAttack = std::exp(-1.0f / (0.008f * static_cast<float>(currentSampleRate)));
    const auto duckRelease = std::exp(-1.0f / (0.180f * static_cast<float>(currentSampleRate)));

    const auto compThresholdDb = juce::jmap(compAmount, 0.0f, 1.0f, 0.0f,
        apvts.getRawParameterValue(VoxlineParameterIDs::compThreshold)->load());
    const auto compRatio = 1.0f
        + (apvts.getRawParameterValue(VoxlineParameterIDs::compRatio)->load() - 1.0f) * compAmount;
    const auto compKneeDb = juce::jmap(compAmount, 0.0f, 1.0f, 6.0f, 2.0f);
    const auto compAttackMs = apvts.getRawParameterValue(VoxlineParameterIDs::compAttack)->load();
    const auto compReleaseMs = apvts.getRawParameterValue(VoxlineParameterIDs::compRelease)->load();
    const auto compAttack = juce::jlimit(0.0f, 1.0f,
        std::exp(-1.0f / static_cast<float>(currentSampleRate * compAttackMs * 0.001f)));
    const auto compRelease = juce::jlimit(0.0f, 1.0f,
        std::exp(-1.0f / static_cast<float>(currentSampleRate * compReleaseMs * 0.001f)));
    const auto compMix = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::compMix)->load());

    const auto spaceTimeScale = juce::jlimit(0.15f, 1.8f, spaceTimeMs / 1200.0f);
    auto spaceMix = spaceAmount * 0.22f;
    auto spaceFeedback = juce::jmap(spaceDecay, 0.1f, 2.5f, 0.04f, 0.56f);
    const auto spaceLpfFreq = juce::jmap(spaceTone, -1.0f, 1.0f, 3200.0f, 14000.0f);
    const auto spaceHpfFreq = juce::jmap(spaceTone, -1.0f, 1.0f, 140.0f, 380.0f);
    const auto spaceHpfCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * spaceHpfFreq
                                        / static_cast<float>(currentSampleRate));
    const auto spaceLpfCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * spaceLpfFreq
                                        / static_cast<float>(currentSampleRate));
    const auto millisecondsToDelaySamples = [this](float milliseconds)
    {
        return juce::jlimit(1, maxSpaceDelaySamples - 1,
            static_cast<int>(currentSampleRate * milliseconds * 0.001f));
    };
    std::array<int, 4> spaceTapDelays {};
    std::array<float, 4> spaceTapGains {};
    auto spaceTapCount = 0;
    auto spaceCrossfeedDelay = 1;
    auto spaceCrossfeedGain = 0.0f;

    switch (spaceType)
    {
    case 0:
        spaceTapCount = 4;
        spaceTapDelays = {
            millisecondsToDelaySamples(spacePreDelayMs + 6.0f * spaceTimeScale),
            millisecondsToDelaySamples(spacePreDelayMs + 14.0f * spaceTimeScale),
            millisecondsToDelaySamples(spacePreDelayMs + 24.0f * spaceTimeScale),
            millisecondsToDelaySamples(spacePreDelayMs + 38.0f * spaceTimeScale)};
        spaceTapGains = {0.40f, 0.30f, 0.20f, 0.10f};
        spaceMix = spaceAmount * 0.18f;
        spaceFeedback *= 0.55f;
        break;
    case 1:
    {
        const auto slapMs = juce::jlimit(55.0f, 320.0f, spaceTimeMs);
        spaceTapCount = 2;
        spaceTapDelays = {
            millisecondsToDelaySamples(spacePreDelayMs + slapMs),
            millisecondsToDelaySamples(spacePreDelayMs + slapMs * 1.52f), 1, 1};
        spaceTapGains = {0.72f, 0.28f, 0.0f, 0.0f};
        spaceMix = spaceAmount * 0.28f;
        break;
    }
    default:
        spaceTapCount = 3;
        spaceTapDelays = {
            millisecondsToDelaySamples(spacePreDelayMs + 12.0f * spaceTimeScale),
            millisecondsToDelaySamples(spacePreDelayMs + 24.0f * spaceTimeScale),
            millisecondsToDelaySamples(spacePreDelayMs + 36.0f * spaceTimeScale), 1};
        spaceTapGains = {0.40f, 0.35f, 0.25f, 0.0f};
        spaceCrossfeedDelay = millisecondsToDelaySamples(
            spacePreDelayMs + juce::jmap(spaceWidth, 0.0f, 2.0f, 7.0f, 28.0f));
        spaceCrossfeedGain = 0.08f + 0.24f * spaceWidth;
        spaceMix = spaceAmount * (0.18f + 0.06f * spaceWidth);
        break;
    }
    auto maxDeEssReductionDb = 0.0f;

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto inputGain = inputGainSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();
        const auto bypassMix = bypassSmoothed.getNextValue();
        const auto spaceBufferPosition = (spaceWritePos + sampleIndex) % maxSpaceDelaySamples;

        float detector = 0.0f;
        for (auto channel = 0; channel < numInputChannels; ++channel)
            detector = juce::jmax(detector, std::abs(buffer.getSample(channel, sampleIndex) * inputGain));

        const auto compressorGain = updateCompressorGain(detector, compAmount, compThresholdDb,
            compRatio, compKneeDb, compAttack, compRelease, compMix);
        const auto duckCoeff = detector > spaceDuckEnvelope ? duckAttack : duckRelease;
        spaceDuckEnvelope = detector + duckCoeff * (spaceDuckEnvelope - detector);
        const auto spaceDuckGain = 1.0f - spaceDucking
            * juce::jlimit(0.0f, 0.82f, spaceDuckEnvelope * 3.2f);

        for (auto channel = 0; channel < numInputChannels; ++channel)
        {
            const auto drySample = dryBuffer.getSample(channel, sampleIndex);
            auto sample = drySample * inputGain;

            // cleanMode: gentle HPF at ~80Hz to remove rumble / low-end mud
            if (cleanModeOn)
            {
                auto& xPrev = cleanModeXPrev[channel];
                auto& yPrev = cleanModeYPrev[channel];
                const auto filtered = cleanModeCoeff * (yPrev + sample - xPrev);
                xPrev = sample;
                yPrev = filtered;
                sample = filtered;
            }

            const auto channelIndex = static_cast<size_t>(channel);
            if (vocalEqEnabled)
            {
                for (int stage = 0; stage < activeHpfStages; ++stage)
                    sample = hpfFilters[channelIndex][static_cast<size_t>(stage)].processSingleSampleRaw(sample);

                sample = lowFilters[channelIndex].processSingleSampleRaw(sample);
                sample = mudFilters[channelIndex].processSingleSampleRaw(sample);
                sample = clarityFilters[channelIndex].processSingleSampleRaw(sample);
                sample = airFilters[channelIndex].processSingleSampleRaw(sample);

                for (int stage = 0; stage < activeLpfStages; ++stage)
                    sample = lpfFilters[channelIndex][static_cast<size_t>(stage)].processSingleSampleRaw(sample);
            }

            // Split-band de-esser. SMOOTH remains the fast amount macro while
            // frequency, threshold, maximum range, and mode live in Advanced.
            if (deEssAmount > 0.0f)
            {
                auto& lowState = deEssLowpassState[channel];
                lowState = sample + deEssCoeff * (lowState - sample);
                const auto highBand = sample - lowState;
                const auto highDb = juce::Decibels::gainToDecibels(std::abs(highBand), -80.0f);
                const auto overDb = juce::jmax(0.0f, highDb - deEssThreshold);
                const auto reductionDb = juce::jmin(deEssRange * deEssAmount, overDb * 0.65f * deEssAmount);
                maxDeEssReductionDb = juce::jmax(maxDeEssReductionDb, reductionDb);
                const auto deEssGain = juce::Decibels::decibelsToGain(-reductionDb);
                sample = deEssWideband ? sample * deEssGain : lowState + highBand * deEssGain;
            }

            sample *= compressorGain;

            if (driveAmount > 0.0f)
            {
                // Conservative vocal saturation:
                // - pre-emphasis nudges upper harmonics into the shaper
                // - asymmetric transfer adds gentle even-order warmth
                // - de-emphasis trims fizz after clipping so high drive stays controlled
                auto& preState = drivePreEmphasisState[channel];
                preState = sample + driveToneCoeff * (preState - sample);
                const auto preEmphasized = sample + (sample - preState) * driveToneAmount;

                auto shaped = preEmphasized * drivePreGain;
                shaped = shaped >= 0.0f ? std::tanh(shaped * 1.08f)
                                         : -std::tanh(-shaped * 0.88f);
                shaped /= driveNormalizer;

                auto& deState = driveDeEmphasisState[channel];
                deState = shaped + driveToneCoeff * (deState - shaped);
                shaped -= (shaped - deState) * driveDeEmphasisAmount;

                const auto driveMix = juce::jlimit(0.0f, 1.0f, driveMixControl * driveAmount);
                sample = juce::jmap(driveMix, sample, shaped);
            }

            // SPACE — Vocal Space Processor
            if (spaceAmount > 0.001f)
            {
                const auto readDelay = [&](int delaySamples, int readChannel = -1) -> float {
                    const auto sourceChannel = readChannel < 0 ? channel : readChannel;
                    return spaceBuffer.getSample(sourceChannel,
                        (spaceBufferPosition - delaySamples + maxSpaceDelaySamples) % maxSpaceDelaySamples);
                };

                float rv = 0.0f;
                for (auto tap = 0; tap < spaceTapCount; ++tap)
                    rv += readDelay(spaceTapDelays[static_cast<size_t>(tap)])
                        * spaceTapGains[static_cast<size_t>(tap)];

                if (spaceType == 2 && spaceBuffer.getNumChannels() > 1)
                    rv += readDelay(spaceCrossfeedDelay, channel == 0 ? 1 : 0) * spaceCrossfeedGain;

                // Simple one-pole HPF and LPF on wet signal
                auto& hpfS = spaceHpfState[channel];
                auto& lpfS = spaceLpfState[channel];
                hpfS = rv + spaceHpfCoeff * (hpfS - rv);
                auto filtered = rv - hpfS;
                lpfS = filtered + spaceLpfCoeff * (lpfS - filtered);
                filtered = lpfS;

                spaceBuffer.setSample(channel, spaceBufferPosition, sample + filtered * spaceFeedback);
                sample = sample + filtered * spaceMix * spaceDuckGain;
            }
            else
            {
                spaceBuffer.setSample(channel, spaceBufferPosition, sample);
            }

            sample = juce::jmap(wetMix, drySample, sample);
            sample *= outputGain;
            sample = applySoftClip(sample);

            if (listenEnabled)
                sample = applySoftClip((sample - drySample) * 2.0f);

            // Bypass crossfade: 0.0=processed, 1.0=dry
            sample = sample + bypassMix * (drySample - sample);

            buffer.setSample(channel, sampleIndex, sample);
        }
    }

    spaceWritePos = (spaceWritePos + numSamples) % maxSpaceDelaySamples;
    deEssReduction.store(maxDeEssReductionDb);

    // Calculate output meters
    float outPeak = 0.0f, outRms = 0.0f;
    for (auto channel = 0; channel < numOutputChannels; ++channel)
    {
        auto* data = buffer.getReadPointer(channel);
        for (auto i = 0; i < numSamples; ++i)
        {
            const auto s = std::abs(data[i]);
            outPeak = juce::jmax(outPeak, s);
            outRms += s * s;
        }
    }
    outRms = std::sqrt(outRms / static_cast<float>(numSamples * numOutputChannels));

    auto writePosition = analyzerWritePosition.load(std::memory_order_relaxed);
    for (auto sample = 0; sample < numSamples; ++sample)
    {
        auto mono = 0.0f;
        for (auto channel = 0; channel < numOutputChannels; ++channel)
            mono += buffer.getSample(channel, sample);
        mono /= static_cast<float>(numOutputChannels);
        analyzerSamples[static_cast<size_t>(writePosition)].store(mono, std::memory_order_relaxed);
        writePosition = (writePosition + 1) % analyzerFftSize;
    }
    analyzerWritePosition.store(writePosition, std::memory_order_release);

    const auto outPeakDb = juce::Decibels::gainToDecibels(outPeak, -60.0f);
    const auto outRmsDb = juce::Decibels::gainToDecibels(outRms, -60.0f);
    auto targetOutPeak = juce::jlimit(0.0f, 1.0f, (outPeakDb + 60.0f) / 60.0f);
    auto targetOutRms = juce::jlimit(0.0f, 1.0f, (outRmsDb + 60.0f) / 60.0f);
    auto prevOutPeak = outputPeak.load();
    auto prevOutRms = outputRms.load();
    auto opCoeff = targetOutPeak > prevOutPeak ? meterAttack : meterRelease;
    auto orCoeff = targetOutRms > prevOutRms ? meterAttack : meterRelease;
    outputPeak.store(prevOutPeak + opCoeff * (targetOutPeak - prevOutPeak));
    outputRms.store(prevOutRms + orCoeff * (targetOutRms - prevOutRms));

    // Gain reduction in dB (from compressor envelope)
    auto grDb = juce::Decibels::gainToDecibels(compressorEnvelope, -24.0f);
    auto targetGr = juce::jlimit(0.0f, 1.0f, -grDb / 24.0f); // 0=no GR, 1=max GR
    auto prevGr = gainReduction.load();
    auto grCoeff = targetGr > prevGr ? meterAttack : meterRelease;
    gainReduction.store(prevGr + grCoeff * (targetGr - prevGr));
}

void VoxlineAudioProcessor::copyAnalyzerSamples(
    std::array<float, analyzerFftSize>& destination) const noexcept
{
    const auto writePosition = analyzerWritePosition.load(std::memory_order_acquire);
    for (int i = 0; i < analyzerFftSize; ++i)
    {
        const auto source = (writePosition + i) % analyzerFftSize;
        destination[static_cast<size_t>(i)] =
            analyzerSamples[static_cast<size_t>(source)].load(std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* VoxlineAudioProcessor::createEditor()
{
    return new VoxlineAudioProcessorEditor(*this);
}

bool VoxlineAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String VoxlineAudioProcessor::getName() const
{
    return "VOXLINE";
}

bool VoxlineAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VoxlineAudioProcessor::producesMidi() const
{
    return false;
}

bool VoxlineAudioProcessor::isMidiEffect() const
{
    return false;
}

double VoxlineAudioProcessor::getTailLengthSeconds() const
{
    return 3.0;
}

int VoxlineAudioProcessor::getNumPrograms()
{
    return 9;
}

int VoxlineAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void VoxlineAudioProcessor::setCurrentProgram(int index)
{
    currentProgram = juce::jlimit(0, getNumPrograms() - 1, index);
    struct PresetDef { float in; bool ag; float pol, bd, cl, ar, sm, cp, dr, out, space; int spaceType; };
    static constexpr PresetDef presets[] = {
        { 0.0f, true, 22, 0.0f, 0.0f, 0.0f, 10, 18, 0, 0.0f, 0, 0 },
        {-1.0f, true, 58, 2.0f, 0.8f,-0.8f, 20, 52,34,-1.5f,12, 1 },
        {-1.5f, true, 78, 1.2f, 3.0f, 0.5f, 24, 76,46,-2.0f, 8, 0 },
        {-1.0f, true, 68,-2.0f, 1.8f, 4.0f, 66, 48,10,-1.5f,20, 2 },
        {-2.0f, true, 86,-1.0f, 4.5f, 2.5f, 22, 84,56,-3.0f,10, 2 },
        {-1.5f, true, 72, 3.5f, 0.3f,-1.8f, 28, 70,48,-2.5f, 6, 1 },
        {-2.0f, true, 88,-2.5f, 4.0f, 5.0f, 38, 78,32,-3.0f,25, 0 },
        {-1.0f, true, 60, 1.0f,-0.8f,-1.5f, 70, 46,18,-1.5f,18, 1 },
        {-1.5f, true, 70, 2.5f, 1.0f,-0.5f, 38, 66,58,-2.5f,12, 2 },
    };
    const auto& preset = presets[static_cast<size_t>(currentProgram)];
    auto setPlain = [this](const char* id, float value)
    {
        if (auto* parameter = apvts.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    setPlain(VoxlineParameterIDs::inputGain, preset.in);
    setPlain(VoxlineParameterIDs::autoGain, preset.ag ? 1.0f : 0.0f);
    setPlain(VoxlineParameterIDs::polish, preset.pol);
    setPlain(VoxlineParameterIDs::body, preset.bd);
    setPlain(VoxlineParameterIDs::clarity, preset.cl);
    setPlain(VoxlineParameterIDs::air, preset.ar);
    setPlain(VoxlineParameterIDs::smooth, preset.sm);
    setPlain(VoxlineParameterIDs::comp, preset.cp);
    setPlain(VoxlineParameterIDs::drive, preset.dr);
    setPlain(VoxlineParameterIDs::outputGain, preset.out);
    setPlain(VoxlineParameterIDs::spaceAmount, preset.space);
    setPlain(VoxlineParameterIDs::spaceType, static_cast<float>(preset.spaceType));
}

const juce::String VoxlineAudioProcessor::getProgramName(int index)
{
    static const juce::StringArray names {
        "Clean", "Basement Take", "Dirty Lead", "Cold Plug", "Rage Cut",
        "Muddy Trap", "Cyber Vox", "Noir Vocal", "Tape Rap"
    };
    return names[juce::jlimit(0, names.size() - 1, index)];
}

void VoxlineAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void VoxlineAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void VoxlineAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xmlState = getXmlFromBinary(data, sizeInBytes);

    if (xmlState == nullptr)
        return;

    if (! xmlState->hasTagName(apvts.state.getType()))
        return;

    apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

VoxlineAudioProcessor::APVTS& VoxlineAudioProcessor::getAPVTS() noexcept
{
    return apvts;
}

const VoxlineAudioProcessor::APVTS& VoxlineAudioProcessor::getAPVTS() const noexcept
{
    return apvts;
}

VoxlineAudioProcessor::APVTS::ParameterLayout VoxlineAudioProcessor::createParameterLayout()
{
    auto params = std::vector<std::unique_ptr<juce::RangedAudioParameter>>{};
    const auto gainRange = juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f};
    const auto percentRange = juce::NormalisableRange<float>{0.0f, 100.0f, 1.0f};
    const auto toneGainRange = juce::NormalisableRange<float>{-6.0f, 6.0f, 0.1f};

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::inputGain, 1}, "Input Gain", gainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::autoGain, 1}, "Auto Gain", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::polish, 1}, "Polish", percentRange, 65.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::body, 1}, "Body", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::clarity, 1}, "Presence", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::air, 1}, "Air", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::smooth, 1}, "Smooth", percentRange, 32.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::comp, 1}, "Comp", percentRange, 42.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::drive, 1}, "Drive", percentRange, 18.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::outputGain, 1}, "Output Gain", gainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::bypass, 1}, "Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::cleanMode, 1}, "Clean Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::listen, 1}, "Listen", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceAmount, 1}, "Space Amount", percentRange, 0.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{VoxlineParameterIDs::spaceType, 1}, "Space Type", 0, 2, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceTime, 1}, "Space Time",
        juce::NormalisableRange<float>{40.0f, 2000.0f, 1.0f, 0.42f}, 1200.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spacePreDelay, 1}, "Space Pre-delay",
        juce::NormalisableRange<float>{0.0f, 120.0f, 1.0f}, 28.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceWidth, 1}, "Space Width",
        juce::NormalisableRange<float>{0.0f, 200.0f, 1.0f}, 135.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceTone, 1}, "Space Tone",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 1.0f}, 12.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceDecay, 1}, "Space Decay",
        juce::NormalisableRange<float>{0.1f, 2.5f, 0.01f, 0.55f}, 1.6f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceDucking, 1}, "Space Ducking",
        percentRange, 42.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::hpfFreq, 1}, "HPF Freq", juce::NormalisableRange<float>{20.0f, 300.0f, 1.0f}, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::hpfSlope, 1}, "HPF Slope", juce::StringArray{"12", "24", "36", "48"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudAmount, 1}, "Mud Amount", percentRange, 0.0f, makePercentAttributes()));

    // Vocal EQ
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::eqEnabled, 1}, "EQ Enabled", true));
    // LOW
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowFreq, 1}, "Low Freq", juce::NormalisableRange<float>{80.0f, 250.0f, 1.0f}, 160.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowGain, 1}, "Low Gain", juce::NormalisableRange<float>{-6.0f, 6.0f, 0.1f}, 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowQ, 1}, "Low Q", juce::NormalisableRange<float>{0.4f, 2.0f, 0.05f}, 0.8f));
    // MUD
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudFreq, 1}, "Mud Freq", juce::NormalisableRange<float>{200.0f, 700.0f, 1.0f}, 350.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudGain, 1}, "Mud Gain", juce::NormalisableRange<float>{-6.0f, 3.0f, 0.1f}, -2.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudQ, 1}, "Mud Q", juce::NormalisableRange<float>{0.5f, 3.0f, 0.05f}, 1.1f));
    // PRES
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presFreq, 1}, "Pres Freq", juce::NormalisableRange<float>{1000.0f, 5000.0f, 10.0f}, 2500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presGain, 1}, "Pres Gain", juce::NormalisableRange<float>{-3.0f, 6.0f, 0.1f}, 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presQ, 1}, "Pres Q", juce::NormalisableRange<float>{0.5f, 3.0f, 0.05f}, 1.0f));
    // AIR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airFreq, 1}, "Air Freq", juce::NormalisableRange<float>{6000.0f, 16000.0f, 100.0f}, 10000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airGain, 1}, "Air Gain", juce::NormalisableRange<float>{-3.0f, 6.0f, 0.1f}, 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airQ, 1}, "Air Q", juce::NormalisableRange<float>{0.4f, 2.0f, 0.05f}, 0.7f));
    // LPF
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lpfFreq, 1}, "LPF Freq", juce::NormalisableRange<float>{8000.0f, 20000.0f, 100.0f}, 18000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::lpfSlope, 1}, "LPF Slope", juce::StringArray{"12", "24"}, 0));

    // Advanced compressor controls. COMP remains the overall intensity macro.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compThreshold, 1}, "Comp Threshold",
        juce::NormalisableRange<float>{-40.0f, 0.0f, 0.1f}, -18.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compRatio, 1}, "Comp Ratio",
        juce::NormalisableRange<float>{1.0f, 10.0f, 0.1f}, 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compAttack, 1}, "Comp Attack",
        juce::NormalisableRange<float>{0.5f, 100.0f, 0.5f, 0.45f}, 15.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compRelease, 1}, "Comp Release",
        juce::NormalisableRange<float>{20.0f, 500.0f, 1.0f, 0.45f}, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compMix, 1}, "Comp Mix", percentRange, 100.0f, makePercentAttributes()));

    // Advanced de-esser controls. SMOOTH is presented as DE-ESS on the main UI.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssFreq, 1}, "De-Ess Frequency",
        juce::NormalisableRange<float>{3000.0f, 12000.0f, 10.0f, 0.45f}, 6500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssThreshold, 1}, "De-Ess Threshold",
        juce::NormalisableRange<float>{-40.0f, 0.0f, 0.1f}, -18.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssRange, 1}, "De-Ess Range",
        juce::NormalisableRange<float>{0.0f, 12.0f, 0.1f}, 6.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::deEssMode, 1}, "De-Ess Mode",
        juce::StringArray{"Split", "Wide"}, 0));

    // Advanced saturation controls. DRIVE remains the overall intensity macro.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::driveTone, 1}, "Drive Tone",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 1.0f}, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::driveMix, 1}, "Drive Mix", percentRange, 70.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::driveCharacter, 1}, "Drive Character",
        juce::StringArray{"Clean", "Warm", "Edge"}, 1));

    return {params.begin(), params.end()};
}

void VoxlineAudioProcessor::updateToneFilters()
{
    // Tone is now owned by the dedicated Vocal EQ and the split-band de-esser.
    // Keeping these legacy filters inactive avoids double-processing old sessions.
    for (auto& filter : bodyFilters) filter.makeInactive();
    for (auto& filter : clarityFilters) filter.makeInactive();
    for (auto& filter : airFilters) filter.makeInactive();
    for (auto& filter : smoothFilters) filter.makeInactive();
}

void VoxlineAudioProcessor::updateEQFilters()
{
    const auto eqOn = apvts.getRawParameterValue(VoxlineParameterIDs::eqEnabled)->load() > 0.5f;

    // updateToneFilters() runs immediately before this method.  When Vocal EQ is disabled,
    // leave BODY/CLARITY/AIR/SMOOTH tone filters intact and bypass only the dedicated EQ stages.
    if (!eqOn)
    {
        for (auto& channelFilters : hpfFilters)
            for (auto& filter : channelFilters)
                filter.makeInactive();
        for (auto& filter : lowFilters) filter.makeInactive();
        for (auto& filter : mudFilters) filter.makeInactive();
        for (auto& channelFilters : lpfFilters)
            for (auto& filter : channelFilters)
                filter.makeInactive();
        return;
    }

    const auto polishRaw = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::polish)->load());
    const auto polishScale = juce::jmap(polishRaw, 0.0f, 1.0f, 0.35f, 1.35f);

    // HPF: cascaded 2nd-order stages. 1/2/3/4 stages = 12/24/36/48 dB/oct.
    const auto hpfF = apvts.getRawParameterValue(VoxlineParameterIDs::hpfFreq)->load();
    const auto hpfS = static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::hpfSlope)->load());
    const auto activeHpfStages = juce::jlimit(1, 4, hpfS + 1);
    const auto hpfCoef = juce::IIRCoefficients::makeHighPass(currentSampleRate, hpfF);
    for (auto& channelFilters : hpfFilters)
    {
        for (auto stage = 0; stage < static_cast<int>(channelFilters.size()); ++stage)
        {
            if (stage < activeHpfStages)
                channelFilters[static_cast<size_t>(stage)].setCoefficients(hpfCoef);
            else
                channelFilters[static_cast<size_t>(stage)].makeInactive();
        }
    }

    // LOW bell
    const auto lf = apvts.getRawParameterValue(VoxlineParameterIDs::lowFreq)->load();
    const auto lg = apvts.getRawParameterValue(VoxlineParameterIDs::body)->load() * polishScale;
    const auto lq = apvts.getRawParameterValue(VoxlineParameterIDs::lowQ)->load();
    const auto lowCoef = juce::IIRCoefficients::makePeakFilter(currentSampleRate, lf, lq, juce::Decibels::decibelsToGain(lg));
    for (auto& filter : lowFilters) filter.setCoefficients(lowCoef);

    // MUD bell
    const auto mf = apvts.getRawParameterValue(VoxlineParameterIDs::mudFreq)->load();
    const auto mg = apvts.getRawParameterValue(VoxlineParameterIDs::mudGain)->load() * polishScale;
    const auto mq = apvts.getRawParameterValue(VoxlineParameterIDs::mudQ)->load();
    const auto mudCoef = juce::IIRCoefficients::makePeakFilter(currentSampleRate, mf, mq, juce::Decibels::decibelsToGain(mg));
    for (auto& filter : mudFilters) filter.setCoefficients(mudCoef);

    // PRES bell (reuses clarityFilters after legacy tone CLARITY has been calculated)
    const auto pf = apvts.getRawParameterValue(VoxlineParameterIDs::presFreq)->load();
    const auto pg = apvts.getRawParameterValue(VoxlineParameterIDs::clarity)->load() * polishScale;
    const auto pq = apvts.getRawParameterValue(VoxlineParameterIDs::presQ)->load();
    const auto presCoef = juce::IIRCoefficients::makePeakFilter(currentSampleRate, pf, pq, juce::Decibels::decibelsToGain(pg));
    for (auto& filter : clarityFilters) filter.setCoefficients(presCoef);

    // AIR shelf (reuses airFilters after legacy tone AIR has been calculated)
    const auto af = apvts.getRawParameterValue(VoxlineParameterIDs::airFreq)->load();
    const auto ag = apvts.getRawParameterValue(VoxlineParameterIDs::air)->load() * polishScale;
    const auto aq = apvts.getRawParameterValue(VoxlineParameterIDs::airQ)->load();
    const auto airCoef = juce::IIRCoefficients::makeHighShelf(currentSampleRate, af, aq, juce::Decibels::decibelsToGain(ag));
    for (auto& filter : airFilters) filter.setCoefficients(airCoef);

    // LPF: 1/2 cascaded stages = 12/24 dB/oct.
    const auto lpfF = apvts.getRawParameterValue(VoxlineParameterIDs::lpfFreq)->load();
    const auto lpfS = static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::lpfSlope)->load());
    const auto activeLpfStages = juce::jlimit(1, 2, lpfS + 1);
    const auto lpfCoef = juce::IIRCoefficients::makeLowPass(currentSampleRate, lpfF);
    for (auto& channelFilters : lpfFilters)
    {
        for (auto stage = 0; stage < static_cast<int>(channelFilters.size()); ++stage)
        {
            if (stage < activeLpfStages)
                channelFilters[static_cast<size_t>(stage)].setCoefficients(lpfCoef);
            else
                channelFilters[static_cast<size_t>(stage)].makeInactive();
        }
    }
}

float VoxlineAudioProcessor::updateCompressorGain(float detector, float amount, float thresholdDb,
                                                  float ratio, float kneeDb, float attack,
                                                  float release, float mix)
{
    if (amount <= 0.0f)
        return 1.0f;

    auto targetGain = 1.0f;
    const auto detectorDb = juce::Decibels::gainToDecibels(detector, thresholdDb);

    if (detectorDb > thresholdDb + kneeDb * 0.5f)
    {
        // Above knee — full compression
        const auto compressedDb = thresholdDb + (detectorDb - thresholdDb) / ratio;
        targetGain = juce::Decibels::decibelsToGain(compressedDb - detectorDb);
    }
    else if (detectorDb > thresholdDb - kneeDb * 0.5f)
    {
        // In knee — smooth transition
        const auto kneePos = (detectorDb - thresholdDb + kneeDb * 0.5f) / kneeDb;
        const auto weightedRatio = 1.0f + (ratio - 1.0f) * kneePos * kneePos; // quadratic fade-in
        const auto compressedDb = thresholdDb + (detectorDb - thresholdDb) / weightedRatio;
        targetGain = juce::Decibels::decibelsToGain(compressedDb - detectorDb);
    }

    const auto coefficient = targetGain < compressorEnvelope ? attack : release;

    compressorEnvelope = targetGain + coefficient * (compressorEnvelope - targetGain);
    return 1.0f + (compressorEnvelope - 1.0f) * mix;
}

float VoxlineAudioProcessor::applySoftClip(float sample) noexcept
{
    if (std::abs(sample) <= 0.98f)
        return sample;

    return std::tanh(sample);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxlineAudioProcessor();
}
