#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxlineAudioProcessor::VoxlineAudioProcessor()
    : AudioProcessor(BusesProperties()
    #if ! JucePlugin_IsMidiEffect
     #if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
     #endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    #endif
      )
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
    return JucePlugin_Name;
}

bool VoxlineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VoxlineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VoxlineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
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
    juce::ignoreUnused(destData);
}

void VoxlineAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxlineAudioProcessor();
}
