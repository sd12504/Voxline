#pragma once

#include <JuceHeader.h>
#include "Parameters/ParameterIDs.h"

class VoxlineAudioProcessor final : public juce::AudioProcessor
{
public:
    VoxlineAudioProcessor();
    ~VoxlineAudioProcessor() override = default;

    using APVTS = juce::AudioProcessorValueTreeState;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    APVTS& getAPVTS() noexcept;
    const APVTS& getAPVTS() const noexcept;

    // Meter values
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> inputRms { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> outputRms { 0.0f };
    std::atomic<float> gainReduction { 0.0f };
    std::atomic<float> deEssReduction { 0.0f };
    static constexpr int analyzerFftOrder = 11;
    static constexpr int analyzerFftSize = 1 << analyzerFftOrder;
    void copyAnalyzerSamples(std::array<float, analyzerFftSize>& destination) const noexcept;

private:
    void updateToneFilters();
    void updateEQFilters();
    float updateCompressorGain(float detector, float amount, float thresholdDb, float ratio,
                               float kneeDb, float attack, float release, float mix);
    static float applySoftClip(float sample) noexcept;

    APVTS apvts;
    juce::SmoothedValue<float> inputGainSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> bypassSmoothed;

    // Tone filters (shared between legacy tone and new EQ UI)
    std::array<juce::IIRFilter, 2> bodyFilters;
    std::array<juce::IIRFilter, 2> clarityFilters;
    std::array<juce::IIRFilter, 2> airFilters;
    std::array<juce::IIRFilter, 2> smoothFilters;
    // EQ slope filters are cascaded 2nd-order stages:
    // 1/2/3/4 active HPF stages = 12/24/36/48 dB/oct, 1/2 active LPF stages = 12/24 dB/oct.
    std::array<std::array<juce::IIRFilter, 4>, 2> hpfFilters;
    std::array<juce::IIRFilter, 2> mudFilters;
    std::array<std::array<juce::IIRFilter, 2>, 2> lpfFilters;   // new: LPF for EQ
    std::array<juce::IIRFilter, 2> lowFilters;   // new: dedicated LOW bell

    juce::AudioBuffer<float> dryBuffer;
    double currentSampleRate = 44100.0;
    float compressorEnvelope = 1.0f;

    float cleanModeXPrev[2] = {0.0f, 0.0f};
    float cleanModeYPrev[2] = {0.0f, 0.0f};
    float drivePreEmphasisState[2] = {0.0f, 0.0f};
    float driveDeEmphasisState[2] = {0.0f, 0.0f};
    float deEssLowpassState[2] = {0.0f, 0.0f};

    juce::AudioBuffer<float> spaceBuffer;
    int spaceWritePos = 0;
    int maxSpaceDelaySamples = 1;
    float spaceHpfState[2] = {0.0f, 0.0f};
    float spaceLpfState[2] = {0.0f, 0.0f};
    float spaceDuckEnvelope = 0.0f;
    std::array<std::atomic<float>, analyzerFftSize> analyzerSamples {};
    std::atomic<int> analyzerWritePosition { 0 };
    int currentProgram = 0;
};
