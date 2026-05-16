#pragma once

#include <JuceHeader.h>

namespace VoxlineParameterIDs
{
static constexpr auto inputGain = "inputGain";
static constexpr auto autoGain = "autoGain";
static constexpr auto polish = "polish";
static constexpr auto body = "body";
static constexpr auto clarity = "clarity";
static constexpr auto air = "air";
static constexpr auto smooth = "smooth";
static constexpr auto comp = "comp";
static constexpr auto drive = "drive";
static constexpr auto outputGain = "outputGain";
static constexpr auto bypass = "bypass";
static constexpr auto cleanMode = "cleanMode";
static constexpr auto listen = "listen";
static constexpr auto spaceAmount = "spaceAmount";
static constexpr auto spaceType = "spaceType";
} // namespace VoxlineParameterIDs

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

    static APVTS::ParameterLayout createParameterLayout();

    // Meter values (updated in processBlock, read from editor timer)
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> inputRms { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> outputRms { 0.0f };
    std::atomic<float> gainReduction { 0.0f }; // in dB

private:
    void updateToneFilters();
    float updateCompressorGain(float detector, float amount);
    static float applySoftClip(float sample) noexcept;

    APVTS apvts;
    juce::SmoothedValue<float> inputGainSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> bypassSmoothed;
    std::array<juce::IIRFilter, 2> bodyFilters;
    std::array<juce::IIRFilter, 2> clarityFilters;
    std::array<juce::IIRFilter, 2> airFilters;
    std::array<juce::IIRFilter, 2> smoothFilters;
    juce::AudioBuffer<float> dryBuffer;
    double currentSampleRate = 44100.0;
    float compressorEnvelope = 1.0f;

    // cleanMode HPF state (one-pole, per channel)
    float cleanModeXPrev[2] = {0.0f, 0.0f};
    float cleanModeYPrev[2] = {0.0f, 0.0f};

    // SPACE effect: delay lines (mono or stereo)
    juce::AudioBuffer<float> spaceBuffer;
    int spaceWritePos = 0;
    static constexpr int maxSpaceDelaySamples = 9600; // ~200ms @ 48kHz
    float spaceHpfState[2] = {0.0f, 0.0f}; // per-channel HPF state
    float spaceLpfState[2] = {0.0f, 0.0f}; // per-channel LPF state
};
