#include "PluginProcessor.h"

#include "Parameters/ParameterLayout.h"
#include "PluginEditor.h"
#include "State/StateSchema.h"

#include <cmath>

namespace
{
float unitFromPercent(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value * 0.01f);
}

float rawValue(const VoxlineAudioProcessor::APVTS& state,
               const char* parameterId) noexcept
{
    const auto* value = state.getRawParameterValue(parameterId);
    jassert(value != nullptr);
    return value != nullptr ? value->load(std::memory_order_relaxed) : 0.0f;
}

bool rawBool(const VoxlineAudioProcessor::APVTS& state,
             const char* parameterId) noexcept
{
    return rawValue(state, parameterId) >= 0.5f;
}

Voxline::Dsp::FilterSlope slopeFromIndex(float value) noexcept
{
    return static_cast<Voxline::Dsp::FilterSlope>(
        juce::jlimit(0, 3, juce::roundToInt(value)));
}

Voxline::Dsp::DriveCharacter driveCharacterFromIndex(float value) noexcept
{
    return static_cast<Voxline::Dsp::DriveCharacter>(
        juce::jlimit(0, 2, juce::roundToInt(value)));
}

Voxline::Dsp::SpaceMode spaceModeFromIndex(float value) noexcept
{
    return static_cast<Voxline::Dsp::SpaceMode>(
        juce::jlimit(0, 4, juce::roundToInt(value)));
}

float normaliseMeterDb(float db) noexcept
{
    return juce::jlimit(
        0.0f,
        1.0f,
        (db - Voxline::Dsp::silenceFloorDbfs)
            / -Voxline::Dsp::silenceFloorDbfs);
}

float maximumChannelValue(
    const Voxline::Dsp::MeterFrame& frame,
    float Voxline::Dsp::ChannelMeter::* member) noexcept
{
    auto value = Voxline::Dsp::silenceFloorDbfs;
    for (auto channel = 0; channel < frame.channelCount; ++channel)
        value = juce::jmax(
            value,
            frame.channels[static_cast<size_t>(channel)].*member);
    return value;
}

struct ParameterSnapshot
{
    float inputGainDb {};
    float outputGainDb {};
    bool bypass {};
    Voxline::Dsp::VocalEqSettings eq;
    Voxline::Dsp::DeEsserSettings deEss;
    Voxline::Dsp::CompressorSettings compressor;
    Voxline::Dsp::PolishSettings polish;
    Voxline::Dsp::DriveSettings drive;
    Voxline::Dsp::SpaceSettings space;
};

ParameterSnapshot captureParameters(
    const VoxlineAudioProcessor::APVTS& state) noexcept
{
    ParameterSnapshot snapshot;
    snapshot.inputGainDb =
        rawValue(state, VoxlineParameterIDs::inputGain);
    snapshot.outputGainDb =
        rawValue(state, VoxlineParameterIDs::outputGain);
    snapshot.bypass =
        rawBool(state, VoxlineParameterIDs::bypass);

    snapshot.eq.enabled =
        rawBool(state, VoxlineParameterIDs::eqEnabled);
    snapshot.eq.hpf = {
        rawBool(state, VoxlineParameterIDs::hpfEnabled),
        rawValue(state, VoxlineParameterIDs::hpfFreq),
        0.0f,
        0.707f};
    snapshot.eq.low = {
        rawBool(state, VoxlineParameterIDs::lowEnabled),
        rawValue(state, VoxlineParameterIDs::lowFreq),
        rawValue(state, VoxlineParameterIDs::body),
        rawValue(state, VoxlineParameterIDs::lowQ)};
    snapshot.eq.mud = {
        rawBool(state, VoxlineParameterIDs::mudEnabled),
        rawValue(state, VoxlineParameterIDs::mudFreq),
        rawValue(state, VoxlineParameterIDs::mudGain),
        rawValue(state, VoxlineParameterIDs::mudQ)};
    snapshot.eq.presence = {
        rawBool(state, VoxlineParameterIDs::presEnabled),
        rawValue(state, VoxlineParameterIDs::presFreq),
        rawValue(state, VoxlineParameterIDs::clarity),
        rawValue(state, VoxlineParameterIDs::presQ)};
    snapshot.eq.air = {
        rawBool(state, VoxlineParameterIDs::airEnabled),
        rawValue(state, VoxlineParameterIDs::airFreq),
        rawValue(state, VoxlineParameterIDs::air),
        rawValue(state, VoxlineParameterIDs::airQ)};
    snapshot.eq.lpf = {
        rawBool(state, VoxlineParameterIDs::lpfEnabled),
        rawValue(state, VoxlineParameterIDs::lpfFreq),
        0.0f,
        0.707f};
    snapshot.eq.hpfSlope =
        slopeFromIndex(rawValue(state, VoxlineParameterIDs::hpfSlope));
    snapshot.eq.lpfSlope =
        slopeFromIndex(rawValue(state, VoxlineParameterIDs::lpfSlope));

    snapshot.deEss.amount =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::smooth));
    snapshot.deEss.focusHz =
        rawValue(state, VoxlineParameterIDs::deEssFreq);
    snapshot.deEss.sensitivityDb =
        rawValue(state, VoxlineParameterIDs::deEssThreshold);
    snapshot.deEss.maxRangeDb =
        rawValue(state, VoxlineParameterIDs::deEssRange);
    snapshot.deEss.mode =
        rawValue(state, VoxlineParameterIDs::deEssMode) >= 0.5f
            ? Voxline::Dsp::DeEssMode::wide
            : Voxline::Dsp::DeEssMode::split;

    snapshot.compressor.amount =
        rawValue(state, VoxlineParameterIDs::comp);
    snapshot.compressor.sensitivity =
        rawValue(state, VoxlineParameterIDs::compSensitivity);
    snapshot.compressor.ratio =
        rawValue(state, VoxlineParameterIDs::compRatio);
    snapshot.compressor.attackMs =
        rawValue(state, VoxlineParameterIDs::compAttack);
    snapshot.compressor.releaseMs =
        rawValue(state, VoxlineParameterIDs::compRelease);
    snapshot.compressor.mix =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::compMix));
    snapshot.compressor.makeupDb =
        rawValue(state, VoxlineParameterIDs::compMakeup);
    snapshot.compressor.autoMakeup =
        rawBool(state, VoxlineParameterIDs::compAutoMakeup);

    snapshot.polish.amount =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::polish));

    snapshot.drive.amount =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::drive));
    snapshot.drive.character =
        driveCharacterFromIndex(
            rawValue(state, VoxlineParameterIDs::driveCharacter));
    snapshot.drive.tone =
        rawValue(state, VoxlineParameterIDs::driveTone) * 0.01f;
    snapshot.drive.mix =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::driveMix));
    snapshot.drive.outputTrimDb =
        rawValue(state, VoxlineParameterIDs::driveOutputTrim);
    snapshot.drive.levelMatch =
        rawBool(state, VoxlineParameterIDs::driveLevelMatch);

    snapshot.space.amount =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::spaceAmount));
    snapshot.space.mode =
        spaceModeFromIndex(rawValue(state, VoxlineParameterIDs::spaceMode));
    snapshot.space.preDelayMs =
        snapshot.space.mode == Voxline::Dsp::SpaceMode::slap
            ? rawValue(state, VoxlineParameterIDs::spaceSlapTime)
            : rawValue(state, VoxlineParameterIDs::spacePreDelay);
    snapshot.space.sizeOrTime =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::spaceSize));
    snapshot.space.decaySeconds =
        rawValue(state, VoxlineParameterIDs::spaceDecay);
    snapshot.space.tone =
        rawValue(state, VoxlineParameterIDs::spaceTone) * 0.01f;
    snapshot.space.width =
        rawValue(state, VoxlineParameterIDs::spaceWidth) * 0.01f;
    snapshot.space.ducking =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::spaceDucking));
    snapshot.space.feedback =
        unitFromPercent(rawValue(state, VoxlineParameterIDs::spaceFeedback));
    snapshot.space.monoSafety =
        rawBool(state, VoxlineParameterIDs::spaceMonoSafety);

    return snapshot;
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
      apvts(*this, nullptr, "VOXLINEState", createVoxlineParameterLayout())
{
}

void VoxlineAudioProcessor::prepareToPlay(double sampleRate,
                                          int samplesPerBlock)
{
    const auto rate = juce::jmax(1.0, sampleRate);
    preparedMaximumBlockSize = juce::jmax(1, samplesPerBlock);
    preparedChannels = juce::jlimit(
        1, 2, juce::jmax(getTotalNumInputChannels(),
                         getTotalNumOutputChannels()));
    const auto spec = Voxline::Dsp::ModuleSpec {
        rate, preparedMaximumBlockSize, preparedChannels};

    inputMeter.prepare(spec);
    vocalEq.prepare(spec);
    deEsser.prepare(spec);
    compressor.prepare(spec);
    polish.prepare(spec);
    drive.prepare(spec);
    space.prepare(spec);
    outputSafety.reset();
    outputMeter.prepare(spec);

    setLatencySamples(drive.latencySamples());
    prepareBypassDelay(preparedChannels,
                       preparedMaximumBlockSize,
                       getLatencySamples());

    spaceSidechainBuffer.setSize(
        preparedChannels,
        preparedMaximumBlockSize,
        false,
        true,
        false);
    spaceSidechainBuffer.clear();

    const auto parameters = captureParameters(apvts);
    inputGainSmoothed.reset(rate, 0.020);
    outputGainSmoothed.reset(rate, 0.020);
    bypassSmoothed.reset(rate, 0.005);
    spaceTailAmountSmoothed.reset(rate, 0.010);
    inputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(parameters.inputGainDb));
    outputGainSmoothed.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(parameters.outputGainDb));
    bypassSmoothed.setCurrentAndTargetValue(parameters.bypass ? 1.0f : 0.0f);
    spaceTailAmountSmoothed.setCurrentAndTargetValue(parameters.space.amount);

    vocalEq.setTargetSettings(parameters.eq);
    deEsser.setTargetSettings(parameters.deEss);
    compressor.setTargetSettings(parameters.compressor);
    polish.setTargetSettings(parameters.polish);
    drive.setTargetSettings(parameters.drive);
    space.setTargetSettings(parameters.space);

    const auto empty = Voxline::Dsp::MeterFrame {};
    inputMeterSnapshot.store(empty);
    outputMeterSnapshot.store(empty);
    compressorReductionDb.store(0.0f, std::memory_order_relaxed);
    outputSafetyActive.store(false, std::memory_order_relaxed);
    reportedTailSeconds.store(
        !parameters.bypass && parameters.space.amount > 0.0f
            ? space.tailSeconds()
            : 0.0,
        std::memory_order_relaxed);
    inputPeak.store(0.0f, std::memory_order_relaxed);
    inputRms.store(0.0f, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
    outputRms.store(0.0f, std::memory_order_relaxed);
    gainReduction.store(0.0f, std::memory_order_relaxed);
    deEssReduction.store(0.0f, std::memory_order_relaxed);
    analyzerWritePosition.store(0, std::memory_order_relaxed);
    for (auto& sample : analyzerSamples)
        sample.store(0.0f, std::memory_order_relaxed);
}

void VoxlineAudioProcessor::releaseResources()
{
}

#if ! JucePlugin_IsMidiEffect
bool VoxlineAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
   #if JucePlugin_IsSynth
    juce::ignoreUnused(layouts);
    return true;
   #else
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (output != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}
#endif

void VoxlineAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();
    const auto samples = buffer.getNumSamples();
    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear(channel, 0, samples);

    if (inputChannels <= 0 || outputChannels <= 0 || samples <= 0)
        return;

    jassert(samples <= preparedMaximumBlockSize);
    jassert(inputChannels <= preparedChannels);
    if (samples > preparedMaximumBlockSize
        || inputChannels > preparedChannels)
    {
        return;
    }

    const auto parameters = captureParameters(apvts);
    inputGainSmoothed.setTargetValue(
        juce::Decibels::decibelsToGain(parameters.inputGainDb));
    outputGainSmoothed.setTargetValue(
        juce::Decibels::decibelsToGain(parameters.outputGainDb));
    bypassSmoothed.setTargetValue(parameters.bypass ? 1.0f : 0.0f);
    spaceTailAmountSmoothed.setTargetValue(parameters.space.amount);
    vocalEq.setTargetSettings(parameters.eq);
    deEsser.setTargetSettings(parameters.deEss);
    compressor.setTargetSettings(parameters.compressor);
    polish.setTargetSettings(parameters.polish);
    drive.setTargetSettings(parameters.drive);
    space.setTargetSettings(parameters.space);

    captureLatencyAlignedDry(buffer);

    for (auto sample = 0; sample < samples; ++sample)
    {
        const auto gain = inputGainSmoothed.getNextValue();
        for (auto channel = 0; channel < inputChannels; ++channel)
            buffer.setSample(channel,
                             sample,
                             buffer.getSample(channel, sample) * gain);
    }

    const auto inputFrame = inputMeter.measureBlock(buffer);
    vocalEq.process(buffer);
    const auto deEssMetrics =
        deEsser.process(buffer, Voxline::Dsp::DeEssMonitor::off);
    const auto compressorMetrics = compressor.process(buffer);
    polish.process(buffer);
    drive.process(buffer);

    for (auto channel = 0; channel < inputChannels; ++channel)
        spaceSidechainBuffer.copyFrom(
            channel, 0, buffer, channel, 0, samples);
    space.process(buffer, spaceSidechainBuffer);

    for (auto sample = 0; sample < samples; ++sample)
    {
        const auto gain = outputGainSmoothed.getNextValue();
        for (auto channel = 0; channel < outputChannels; ++channel)
            buffer.setSample(channel,
                             sample,
                             buffer.getSample(channel, sample) * gain);
    }

    outputSafety.process(buffer);
    const auto safetyProcessed = outputSafety.wasActive();

    auto maximumWetContribution = 0.0f;
    auto maximumSpaceTailAmount = 0.0f;
    for (auto sample = 0; sample < samples; ++sample)
    {
        const auto bypass = bypassSmoothed.getNextValue();
        const auto wetContribution = 1.0f - bypass;
        maximumWetContribution =
            juce::jmax(maximumWetContribution, wetContribution);
        maximumSpaceTailAmount =
            juce::jmax(maximumSpaceTailAmount,
                       spaceTailAmountSmoothed.getNextValue());
        for (auto channel = 0; channel < outputChannels; ++channel)
        {
            const auto processed = buffer.getSample(channel, sample);
            const auto dry = bypassDryBuffer.getSample(channel, sample);
            buffer.setSample(
                channel, sample, processed + bypass * (dry - processed));
        }
    }
    outputSafetyActive.store(
        safetyProcessed && maximumWetContribution > 1.0e-5f,
        std::memory_order_release);
    reportedTailSeconds.store(
        maximumWetContribution > 1.0e-5f
                && maximumSpaceTailAmount > 1.0e-5f
            ? space.tailSeconds()
            : 0.0,
        std::memory_order_release);

    const auto outputFrame = outputMeter.measureBlock(buffer);
    publishMeters(inputFrame, outputFrame);
    compressorReductionDb.store(
        compressorMetrics.gainReductionDb, std::memory_order_release);
    gainReduction.store(
        juce::jlimit(0.0f,
                     1.0f,
                     compressorMetrics.gainReductionDb / 24.0f),
        std::memory_order_relaxed);
    deEssReduction.store(
        deEssMetrics.reductionDb, std::memory_order_release);
    writeAnalyzer(buffer);
}

void VoxlineAudioProcessor::copyAnalyzerSamples(
    std::array<float, analyzerFftSize>& destination) const noexcept
{
    const auto writePosition =
        analyzerWritePosition.load(std::memory_order_acquire);
    for (auto index = 0; index < analyzerFftSize; ++index)
    {
        const auto source = (writePosition + index) % analyzerFftSize;
        destination[static_cast<size_t>(index)] =
            analyzerSamples[static_cast<size_t>(source)]
                .load(std::memory_order_relaxed);
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
    return reportedTailSeconds.load(std::memory_order_acquire);
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
    return "Default";
}

void VoxlineAudioProcessor::changeProgramName(
    int,
    const juce::String&)
{
}

void VoxlineAudioProcessor::getStateInformation(
    juce::MemoryBlock& destination)
{
    VoxlineState::serialise(apvts.copyState(), destination);
}

void VoxlineAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    if (auto state = VoxlineState::deserialise(
            data, sizeInBytes, apvts.state.getType()))
        apvts.replaceState(*state);
}

VoxlineAudioProcessor::APVTS&
VoxlineAudioProcessor::getAPVTS() noexcept
{
    return apvts;
}

const VoxlineAudioProcessor::APVTS&
VoxlineAudioProcessor::getAPVTS() const noexcept
{
    return apvts;
}

Voxline::Dsp::MeterFrame
VoxlineAudioProcessor::getInputMeterFrame() const noexcept
{
    return inputMeterSnapshot.load();
}

Voxline::Dsp::MeterFrame
VoxlineAudioProcessor::getOutputMeterFrame() const noexcept
{
    return outputMeterSnapshot.load();
}

float VoxlineAudioProcessor::getGainReductionDb() const noexcept
{
    return compressorReductionDb.load(std::memory_order_acquire);
}

bool VoxlineAudioProcessor::wasOutputSafetyActive() const noexcept
{
    return outputSafetyActive.load(std::memory_order_acquire);
}

void VoxlineAudioProcessor::clearOutputClipHold() noexcept
{
    outputMeter.clearClipHold();
}

VoxlineAudioProcessor::PreparedStorageSnapshot
VoxlineAudioProcessor::getPreparedStorageSnapshot() const noexcept
{
    return {
        preparedMaximumBlockSize,
        preparedChannels,
        bypassDelay.empty()
            ? 0
            : static_cast<int>(bypassDelay.front().size()),
        preparedChannels > 0
            ? bypassDryBuffer.getReadPointer(0)
            : nullptr,
        preparedChannels > 0
            ? spaceSidechainBuffer.getReadPointer(0)
            : nullptr,
        bypassDelay.empty()
            ? nullptr
            : bypassDelay.front().data()};
}

void VoxlineAudioProcessor::AtomicMeterFrame::store(
    const Voxline::Dsp::MeterFrame& frame) noexcept
{
    for (size_t channel = 0; channel < peakDbfs.size(); ++channel)
    {
        peakDbfs[channel].store(
            frame.channels[channel].peakDbfs,
            std::memory_order_relaxed);
        rmsDbfs[channel].store(
            frame.channels[channel].rmsDbfs,
            std::memory_order_relaxed);
        truePeakDbtp[channel].store(
            frame.channels[channel].truePeakDbtp,
            std::memory_order_relaxed);
    }
    clipHeld.store(frame.clipHeld, std::memory_order_relaxed);
    channelCount.store(frame.channelCount, std::memory_order_release);
}

Voxline::Dsp::MeterFrame
VoxlineAudioProcessor::AtomicMeterFrame::load() const noexcept
{
    Voxline::Dsp::MeterFrame frame;
    frame.channelCount =
        channelCount.load(std::memory_order_acquire);
    for (size_t channel = 0; channel < peakDbfs.size(); ++channel)
    {
        frame.channels[channel].peakDbfs =
            peakDbfs[channel].load(std::memory_order_relaxed);
        frame.channels[channel].rmsDbfs =
            rmsDbfs[channel].load(std::memory_order_relaxed);
        frame.channels[channel].truePeakDbtp =
            truePeakDbtp[channel].load(std::memory_order_relaxed);
    }
    frame.clipHeld = clipHeld.load(std::memory_order_relaxed);
    return frame;
}

void VoxlineAudioProcessor::prepareBypassDelay(
    int channels,
    int maximumBlockSize,
    int latencySamples)
{
    bypassDryBuffer.setSize(
        channels, maximumBlockSize, false, true, false);
    bypassDryBuffer.clear();
    const auto capacity =
        juce::jmax(1, latencySamples + maximumBlockSize + 1);
    bypassDelay.assign(
        static_cast<size_t>(channels),
        std::vector<float>(static_cast<size_t>(capacity), 0.0f));
    bypassDelayWritePosition = 0;
}

void VoxlineAudioProcessor::captureLatencyAlignedDry(
    const juce::AudioBuffer<float>& input) noexcept
{
    const auto channels =
        juce::jmin(input.getNumChannels(), preparedChannels);
    const auto samples =
        juce::jmin(input.getNumSamples(), preparedMaximumBlockSize);
    const auto capacity = static_cast<int>(bypassDelay.front().size());
    const auto latency = getLatencySamples();

    for (auto sample = 0; sample < samples; ++sample)
    {
        auto readPosition = bypassDelayWritePosition - latency;
        if (readPosition < 0)
            readPosition += capacity;

        for (auto channel = 0; channel < channels; ++channel)
        {
            auto& delay = bypassDelay[static_cast<size_t>(channel)];
            const auto dry = input.getSample(channel, sample);
            bypassDryBuffer.setSample(
                channel,
                sample,
                latency == 0
                    ? dry
                    : delay[static_cast<size_t>(readPosition)]);
            delay[static_cast<size_t>(bypassDelayWritePosition)] = dry;
        }

        if (++bypassDelayWritePosition == capacity)
            bypassDelayWritePosition = 0;
    }
}

void VoxlineAudioProcessor::publishMeters(
    const Voxline::Dsp::MeterFrame& input,
    const Voxline::Dsp::MeterFrame& output) noexcept
{
    inputMeterSnapshot.store(input);
    outputMeterSnapshot.store(output);

    inputPeak.store(
        normaliseMeterDb(
            maximumChannelValue(
                input, &Voxline::Dsp::ChannelMeter::peakDbfs)),
        std::memory_order_relaxed);
    inputRms.store(
        normaliseMeterDb(
            maximumChannelValue(
                input, &Voxline::Dsp::ChannelMeter::rmsDbfs)),
        std::memory_order_relaxed);
    outputPeak.store(
        normaliseMeterDb(
            maximumChannelValue(
                output, &Voxline::Dsp::ChannelMeter::peakDbfs)),
        std::memory_order_relaxed);
    outputRms.store(
        normaliseMeterDb(
            maximumChannelValue(
                output, &Voxline::Dsp::ChannelMeter::rmsDbfs)),
        std::memory_order_relaxed);
}

void VoxlineAudioProcessor::writeAnalyzer(
    const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = buffer.getNumChannels();
    if (channels <= 0)
        return;

    auto writePosition =
        analyzerWritePosition.load(std::memory_order_relaxed);
    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto mono = 0.0f;
        for (auto channel = 0; channel < channels; ++channel)
            mono += buffer.getSample(channel, sample);
        mono /= static_cast<float>(channels);
        analyzerSamples[static_cast<size_t>(writePosition)]
            .store(mono, std::memory_order_relaxed);
        writePosition = (writePosition + 1) % analyzerFftSize;
    }
    analyzerWritePosition.store(
        writePosition, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxlineAudioProcessor();
}
