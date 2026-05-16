#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    for (auto* filters : { &bodyFilters, &clarityFilters, &airFilters, &smoothFilters })
        for (auto& filter : *filters)
            filter.reset();

    dryBuffer.setSize(juce::jmax(1, getTotalNumInputChannels()), juce::jmax(1, samplesPerBlock), false, false, true);
    spaceBuffer.setSize(juce::jmax(2, getTotalNumOutputChannels()), maxSpaceDelaySamples, false, false, true);
    spaceBuffer.clear();
    spaceWritePos = 0;
    compressorEnvelope = 1.0f;
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

    const auto spaceAmount = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::spaceAmount)->load());
    const auto spaceType = static_cast<int>(apvts.getRawParameterValue(VoxlineParameterIDs::spaceType)->load());

    inputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::inputGain)->load()));

    auto outputGainDb = apvts.getRawParameterValue(VoxlineParameterIDs::outputGain)->load();
    if (autoGainEnabled)
        outputGainDb += compAmount * 2.5f - driveAmount * 1.2f + polishScale * 0.6f;
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));

    updateToneFilters();

    const auto drivePreGain = juce::Decibels::decibelsToGain(driveAmount * 12.0f);
    const auto driveNormalizer = std::tanh(drivePreGain);
    const auto wetMix = juce::jlimit(0.0f, 1.0f, 0.35f + polishScale * 0.30f);

    for (auto sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto inputGain = inputGainSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();

        float detector = 0.0f;
        for (auto channel = 0; channel < numInputChannels; ++channel)
            detector = juce::jmax(detector, std::abs(buffer.getSample(channel, sampleIndex) * inputGain));

        const auto compressorGain = updateCompressorGain(detector, compAmount);

        for (auto channel = 0; channel < numInputChannels; ++channel)
        {
            const auto drySample = dryBuffer.getSample(channel, sampleIndex);
            auto sample = drySample * inputGain;

            sample = bodyFilters[static_cast<size_t>(channel)].processSingleSampleRaw(sample);
            sample = clarityFilters[static_cast<size_t>(channel)].processSingleSampleRaw(sample);
            sample = airFilters[static_cast<size_t>(channel)].processSingleSampleRaw(sample);
            sample = smoothFilters[static_cast<size_t>(channel)].processSingleSampleRaw(sample);
            sample *= compressorGain;

            if (driveAmount > 0.0f)
                sample = std::tanh(sample * drivePreGain) / juce::jmax(0.01f, driveNormalizer);

            // SPACE — Vocal Space Processor
            if (spaceAmount > 0.001f)
            {
                const auto spaceBufSize = maxSpaceDelaySamples;
                const auto wp = (spaceWritePos + sampleIndex) % spaceBufSize;

                auto readMs = [&](float ms) -> float {
                    const auto d = static_cast<int>(currentSampleRate * ms * 0.001f);
                    return spaceBuffer.getSample(channel, (wp - d + spaceBufSize) % spaceBufSize);
                };

                float rv = 0.0f;
                float mix = spaceAmount * 0.22f; // conservative mix
                float fb = 0.15f;
                float lpfFreq = 6000.0f; // high-cut
                float hpfFreq = 220.0f;  // low-cut

                switch (spaceType)
                {
                case 0: // Tight Ambience
                    rv = readMs(16.0f)*0.35f + readMs(28.0f)*0.30f + readMs(42.0f)*0.20f + readMs(56.0f)*0.15f;
                    mix = spaceAmount * 0.18f;
                    fb = 0.18f;
                    lpfFreq = 7000.0f;
                    break;
                case 1: // Filtered Slap
                    rv = readMs(105.0f)*0.6f + readMs(130.0f)*0.4f;
                    mix = spaceAmount * 0.28f;
                    fb = 0.28f;
                    lpfFreq = 4500.0f;
                    break;
                case 2: // Stereo Wide
                    rv = readMs(18.0f)*0.35f + readMs(30.0f)*0.30f + readMs(48.0f)*0.20f + readMs(68.0f)*0.15f;
                    if (spaceBuffer.getNumChannels() > 1)
                    {
                        const auto other = (channel == 0) ? 1 : 0;
                        rv += spaceBuffer.getSample(other, (wp - static_cast<int>(currentSampleRate * 0.022f) + spaceBufSize) % spaceBufSize) * 0.25f;
                    }
                    mix = spaceAmount * 0.24f;
                    fb = 0.20f;
                    lpfFreq = 8000.0f;
                    break;
                default:
                    rv = readMs(20.0f)*0.3f + readMs(35.0f)*0.25f + readMs(50.0f)*0.20f + readMs(68.0f)*0.15f + readMs(85.0f)*0.10f;
                    mix = spaceAmount * 0.22f;
                    fb = 0.22f;
                    break;
                }

                // Simple one-pole HPF and LPF on wet signal
                const auto hpfCoeff = std::exp(-2.0f * 3.14159f * hpfFreq / static_cast<float>(currentSampleRate));
                const auto lpfCoeff = std::exp(-2.0f * 3.14159f * lpfFreq / static_cast<float>(currentSampleRate));
                auto& hpfS = spaceHpfState[channel];
                auto& lpfS = spaceLpfState[channel];
                hpfS = rv + hpfCoeff * (hpfS - rv);
                auto filtered = rv - hpfS;
                lpfS = filtered + lpfCoeff * (lpfS - filtered);
                filtered = lpfS;

                spaceBuffer.setSample(channel, wp, sample + filtered * fb);
                sample = sample + filtered * mix;
            }

            sample = juce::jmap(wetMix, drySample, sample);
            sample *= outputGain;
            sample = applySoftClip(sample);

            if (listenEnabled)
                sample = applySoftClip((sample - drySample) * 2.0f);

            // Bypass crossfade: 0.0=processed, 1.0=dry
            const auto bypassMix = bypassSmoothed.getNextValue();
            sample = sample + bypassMix * (drySample - sample);

            buffer.setSample(channel, sampleIndex, sample);
        }
    }

    spaceWritePos = (spaceWritePos + numSamples) % maxSpaceDelaySamples;

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
    return 0.0;
}

int VoxlineAudioProcessor::getNumPrograms()
{
    return 1;
}

int VoxlineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoxlineAudioProcessor::setCurrentProgram(int)
{
}

const juce::String VoxlineAudioProcessor::getProgramName(int)
{
    return {};
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::inputGain, 1}, "Input Gain", gainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::autoGain, 1}, "Auto Gain", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::polish, 1}, "Polish", percentRange, 65.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::body, 1}, "Body", percentRange, 54.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::clarity, 1}, "Clarity", percentRange, 61.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::air, 1}, "Air", percentRange, 48.0f, makePercentAttributes()));
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
        juce::ParameterID{VoxlineParameterIDs::listen, 1}, "Listen", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceAmount, 1}, "Space Amount", percentRange, 0.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{VoxlineParameterIDs::spaceType, 1}, "Space Type", 0, 3, 0));

    return {params.begin(), params.end()};
}

void VoxlineAudioProcessor::updateToneFilters()
{
    const auto polishRaw = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::polish)->load());
    const auto polishScale = juce::jmap(polishRaw, 0.0f, 1.0f, 0.35f, 1.35f);

    const auto bodyRaw     = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::body)->load());
    const auto clarityRaw  = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::clarity)->load());
    const auto airRaw      = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::air)->load());
    const auto smoothRaw   = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::smooth)->load());

    // Body: bell at 200Hz, -4 to +5 dB, Polish scales
    const auto bodyDb = juce::jmap(bodyRaw, 0.0f, 1.0f, -4.0f, 5.0f) * polishScale;
    const auto bodyCoefficients = juce::IIRCoefficients::makePeakFilter(
        currentSampleRate, 200.0f, 0.6f, juce::Decibels::decibelsToGain(bodyDb));

    // Clarity: bell at 3500Hz, -3 to +6 dB
    const auto clarityDb = juce::jmap(clarityRaw, 0.0f, 1.0f, -3.0f, 6.0f) * polishScale;
    const auto clarityCoefficients = juce::IIRCoefficients::makePeakFilter(
        currentSampleRate, 3500.0f, 1.0f, juce::Decibels::decibelsToGain(clarityDb));

    // Air: high shelf at 7kHz, -3 to +7 dB
    const auto airDb = juce::jmap(airRaw, 0.0f, 1.0f, -3.0f, 7.0f) * polishScale;
    const auto airCoefficients = juce::IIRCoefficients::makeHighShelf(
        currentSampleRate, 7000.0f, 0.7f, juce::Decibels::decibelsToGain(airDb));

    // Smooth: high shelf cut at 6kHz, 0 to -6 dB
    const auto smoothDb = juce::jmap(smoothRaw, 0.0f, 1.0f, 0.0f, -6.0f) * polishScale;
    const auto smoothCoefficients = juce::IIRCoefficients::makeHighShelf(
        currentSampleRate, 6000.0f, 0.7f, juce::Decibels::decibelsToGain(smoothDb));

    for (size_t channel = 0; channel < bodyFilters.size(); ++channel)
    {
        bodyFilters[channel].setCoefficients(bodyCoefficients);
        clarityFilters[channel].setCoefficients(clarityCoefficients);
        airFilters[channel].setCoefficients(airCoefficients);
        smoothFilters[channel].setCoefficients(smoothCoefficients);
    }
}

float VoxlineAudioProcessor::updateCompressorGain(float detector, float amount)
{
    if (amount <= 0.0f)
        return 1.0f;

    const auto thresholdDb = juce::jmap(amount, 0.0f, 1.0f, -12.0f, -32.0f);
    const auto thresholdGain = juce::Decibels::decibelsToGain(thresholdDb);
    const auto ratio = juce::jmap(amount, 0.0f, 1.0f, 1.2f, 5.0f);

    auto targetGain = 1.0f;
    if (detector > thresholdGain)
    {
        const auto detectorDb = juce::Decibels::gainToDecibels(detector, thresholdDb);
        const auto compressedDb = thresholdDb + (detectorDb - thresholdDb) / ratio;
        targetGain = juce::Decibels::decibelsToGain(compressedDb - detectorDb);
    }

    const auto attack = juce::jlimit(0.0f, 1.0f, std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.010)));
    const auto release = juce::jlimit(0.0f, 1.0f, std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.080)));
    const auto coefficient = targetGain < compressorEnvelope ? attack : release;

    compressorEnvelope = targetGain + coefficient * (compressorEnvelope - targetGain);
    return compressorEnvelope;
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
