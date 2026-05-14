#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float percentToUnit(float value) noexcept
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
    inputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::inputGain)->load()));
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::outputGain)->load()));

    for (auto* filters : { &bodyFilters, &clarityFilters, &airFilters, &smoothFilters })
        for (auto& filter : *filters)
            filter.reset();

    dryBuffer.setSize(juce::jmax(1, getTotalNumInputChannels()), juce::jmax(1, samplesPerBlock), false, false, true);
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

    if (apvts.getRawParameterValue(VoxlineParameterIDs::bypass)->load() >= 0.5f)
        return;

    const auto polishAmount = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::polish)->load());
    const auto compAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::comp)->load()) + polishAmount * 0.25f);
    const auto driveAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::drive)->load()) + polishAmount * 0.20f);
    const auto autoGainEnabled = apvts.getRawParameterValue(VoxlineParameterIDs::autoGain)->load() >= 0.5f;
    const auto listenEnabled = apvts.getRawParameterValue(VoxlineParameterIDs::listen)->load() >= 0.5f;

    inputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue(VoxlineParameterIDs::inputGain)->load()));

    auto outputGainDb = apvts.getRawParameterValue(VoxlineParameterIDs::outputGain)->load();
    if (autoGainEnabled)
        outputGainDb += compAmount * 2.5f - driveAmount * 1.2f + polishAmount * 0.8f;
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));

    updateToneFilters();

    const auto drivePreGain = juce::Decibels::decibelsToGain(driveAmount * 18.0f);
    const auto driveNormalizer = std::tanh(drivePreGain);
    const auto wetMix = juce::jlimit(0.0f, 1.0f, 0.35f + polishAmount * 0.25f);

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

            sample = juce::jmap(wetMix, drySample, sample);
            sample *= outputGain;
            sample = applySoftClip(sample);

            if (listenEnabled)
                sample = applySoftClip((sample - drySample) * 2.0f);

            buffer.setSample(channel, sampleIndex, sample);
        }
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

    return {params.begin(), params.end()};
}

void VoxlineAudioProcessor::updateToneFilters()
{
    const auto polishAmount = percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::polish)->load());
    const auto bodyAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::body)->load()) + polishAmount * 0.30f);
    const auto clarityAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::clarity)->load()) + polishAmount * 0.25f);
    const auto airAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::air)->load()) + polishAmount * 0.22f);
    const auto smoothAmount = juce::jlimit(0.0f, 1.0f, percentToUnit(apvts.getRawParameterValue(VoxlineParameterIDs::smooth)->load()) + polishAmount * 0.18f);

    const auto bodyCoefficients = juce::IIRCoefficients::makeLowShelf(
        currentSampleRate, 180.0f, 0.7071f, juce::Decibels::decibelsToGain(bodyAmount * 5.0f));
    const auto clarityCoefficients = juce::IIRCoefficients::makePeakFilter(
        currentSampleRate, 3200.0f, 0.8f, juce::Decibels::decibelsToGain(clarityAmount * 4.0f));
    const auto airCoefficients = juce::IIRCoefficients::makeHighShelf(
        currentSampleRate, 10000.0f, 0.7071f, juce::Decibels::decibelsToGain(airAmount * 6.0f));
    const auto smoothCutoff = juce::jmap(smoothAmount, 0.0f, 1.0f, 20000.0f, 4500.0f);
    const auto smoothCoefficients = juce::IIRCoefficients::makeLowPass(currentSampleRate, smoothCutoff, 0.7071f);

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

    const auto thresholdDb = juce::jmap(amount, 0.0f, 1.0f, -8.0f, -24.0f);
    const auto thresholdGain = juce::Decibels::decibelsToGain(thresholdDb);
    const auto ratio = juce::jmap(amount, 0.0f, 1.0f, 1.2f, 4.5f);

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
