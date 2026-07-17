#pragma once

#include <JuceHeader.h>

#include "DSP/DeEsser.h"
#include "DSP/Metering.h"
#include "DSP/OutputSafety.h"
#include "DSP/VocalCompressor.h"
#include "DSP/VocalDrive.h"
#include "DSP/VocalEq.h"
#include "DSP/VocalPolish.h"
#include "DSP/VocalSpace.h"
#include "Parameters/ParameterIDs.h"
#include "State/ABState.h"
#include "State/MonitorState.h"

#include <array>
#include <atomic>
#include <vector>

class VoxlineAudioProcessor final : public juce::AudioProcessor
{
public:
    VoxlineAudioProcessor();
    ~VoxlineAudioProcessor() override = default;

    using APVTS = juce::AudioProcessorValueTreeState;

    struct PreparedStorageSnapshot
    {
        int maximumBlockSize {};
        int channels {};
        int bypassDelayCapacity {};
        const float* bypassDryChannel0 {};
        const float* spaceSidechainChannel0 {};
        const float* bypassDelayChannel0 {};

        bool operator==(const PreparedStorageSnapshot&) const = default;
    };

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
    VoxlineState::AbStateManager& getAbState() noexcept;
    const VoxlineState::AbStateManager& getAbState() const noexcept;
    VoxlineState::MonitorState& getMonitorState() noexcept;
    const VoxlineState::MonitorState& getMonitorState() const noexcept;

    Voxline::Dsp::MeterFrame getInputMeterFrame() const noexcept;
    Voxline::Dsp::MeterFrame getOutputMeterFrame() const noexcept;
    float getGainReductionDb() const noexcept;
    bool wasOutputSafetyActive() const noexcept;
    void clearOutputClipHold() noexcept;
    PreparedStorageSnapshot getPreparedStorageSnapshot() const noexcept;

    // Compatibility values for the current editor. Meter values remain
    // normalised here until the modular meter UI consumes MeterFrame directly.
    std::atomic<float> inputPeak {0.0f};
    std::atomic<float> inputRms {0.0f};
    std::atomic<float> outputPeak {0.0f};
    std::atomic<float> outputRms {0.0f};
    std::atomic<float> gainReduction {0.0f};
    std::atomic<float> deEssReduction {0.0f};

    static constexpr int analyzerFftOrder = 11;
    static constexpr int analyzerFftSize = 1 << analyzerFftOrder;
    void copyAnalyzerSamples(
        std::array<float, analyzerFftSize>& destination) const noexcept;

private:
    struct AtomicMeterFrame
    {
        void store(const Voxline::Dsp::MeterFrame&) noexcept;
        Voxline::Dsp::MeterFrame load() const noexcept;

        std::array<std::atomic<float>, 2> peakDbfs {};
        std::array<std::atomic<float>, 2> rmsDbfs {};
        std::array<std::atomic<float>, 2> truePeakDbtp {};
        std::atomic<int> channelCount {0};
        std::atomic<bool> clipHeld {false};
    };

    void prepareBypassDelay(int channels,
                            int maximumBlockSize,
                            int latencySamples);
    void captureLatencyAlignedDry(
        const juce::AudioBuffer<float>& input) noexcept;
    void publishMeters(const Voxline::Dsp::MeterFrame& input,
                       const Voxline::Dsp::MeterFrame& output) noexcept;
    void writeAnalyzer(const juce::AudioBuffer<float>&) noexcept;
    void applyLegacySpaceAutomation(
        Voxline::Dsp::SpaceSettings&) noexcept;

    APVTS apvts;
    VoxlineState::AbStateManager abState;
    VoxlineState::MonitorState monitorState;

    Voxline::Dsp::BallisticMeter inputMeter;
    Voxline::Dsp::VocalEq vocalEq;
    Voxline::Dsp::DeEsser deEsser;
    Voxline::Dsp::VocalCompressor compressor;
    Voxline::Dsp::VocalPolish polish;
    Voxline::Dsp::VocalDrive drive;
    Voxline::Dsp::VocalSpace space;
    Voxline::Dsp::EmergencySoftClipper outputSafety;
    Voxline::Dsp::BallisticMeter outputMeter;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        outputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        bypassSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        spaceTailAmountSmoothed;

    juce::AudioBuffer<float> bypassDryBuffer;
    juce::AudioBuffer<float> spaceSidechainBuffer;
    std::vector<std::vector<float>> bypassDelay;
    int bypassDelayWritePosition {};
    int preparedMaximumBlockSize {};
    int preparedChannels {};

    AtomicMeterFrame inputMeterSnapshot;
    AtomicMeterFrame outputMeterSnapshot;
    std::atomic<float> compressorReductionDb {0.0f};
    std::atomic<bool> outputSafetyActive {false};
    std::atomic<double> reportedTailSeconds {0.0};
    std::atomic<bool> legacySpaceBridgeEnabled {false};
    std::atomic<bool> legacySpaceOverrideActive {false};
    std::atomic<float> lastLegacySpaceType {0.0f};
    std::atomic<float> lastLegacySpaceTime {1200.0f};
    std::atomic<int> legacySpaceModeOverride {1};
    std::atomic<float> legacySlapTimeOverride {120.0f};

    std::array<std::atomic<float>, analyzerFftSize> analyzerSamples {};
    std::atomic<int> analyzerWritePosition {0};
};
