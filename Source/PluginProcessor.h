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
static constexpr auto hpfFreq = "hpfFreq";
static constexpr auto mudAmount = "mudAmount";

// Vocal EQ
static constexpr auto eqEnabled   = "eqEnabled";
static constexpr auto hpfSlope    = "hpfSlope";
static constexpr auto lowFreq     = "lowFreq";
static constexpr auto lowGain     = "lowGain";
static constexpr auto lowQ        = "lowQ";
static constexpr auto mudFreq     = "mudFreq";
static constexpr auto mudGain     = "mudGain";
static constexpr auto mudQ        = "mudQ";
static constexpr auto presFreq    = "presFreq";
static constexpr auto presGain    = "presGain";
static constexpr auto presQ       = "presQ";
static constexpr auto airFreq     = "airFreq";
static constexpr auto airGain     = "airGain";
static constexpr auto airQ        = "airQ";
static constexpr auto lpfFreq     = "lpfFreq";
static constexpr auto lpfSlope    = "lpfSlope";
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

    // Meter values
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> inputRms { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> outputRms { 0.0f };
    std::atomic<float> gainReduction { 0.0f };

private:
    void updateToneFilters();
    void updateEQFilters();
    float updateCompressorGain(float detector, float amount);
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
    std::array<juce::IIRFilter, 2> hpfFilters;
    std::array<juce::IIRFilter, 2> mudFilters;
    std::array<juce::IIRFilter, 2> lpfFilters;   // new: LPF for EQ
    std::array<juce::IIRFilter, 2> lowFilters;   // new: dedicated LOW bell

    juce::AudioBuffer<float> dryBuffer;
    double currentSampleRate = 44100.0;
    float compressorEnvelope = 1.0f;

    float cleanModeXPrev[2] = {0.0f, 0.0f};
    float cleanModeYPrev[2] = {0.0f, 0.0f};

    juce::AudioBuffer<float> spaceBuffer;
    int spaceWritePos = 0;
    static constexpr int maxSpaceDelaySamples = 9600;
    float spaceHpfState[2] = {0.0f, 0.0f};
    float spaceLpfState[2] = {0.0f, 0.0f};
};
