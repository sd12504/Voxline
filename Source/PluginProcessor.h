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
static constexpr auto listen = "listen";
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
};
