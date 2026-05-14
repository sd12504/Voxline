#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
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

void VoxlineAudioProcessor::prepareToPlay(double, int)
{
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxlineAudioProcessor();
}
