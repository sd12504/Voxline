#pragma once

#include "DspTypes.h"

#include <array>
#include <memory>
#include <vector>

namespace Voxline::Dsp
{
enum class DriveCharacter
{
    clean,
    warm,
    edge
};

struct DriveSettings
{
    float amount {};
    DriveCharacter character {DriveCharacter::warm};
    float tone {};
    float mix {0.7f};
    float outputTrimDb {};
    bool levelMatch {true};
};

class VocalDrive
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const DriveSettings&) noexcept;
    void process(juce::AudioBuffer<float>&) noexcept;
    int latencySamples() const noexcept;

private:
    static float transfer(float sample,
                          float amount,
                          DriveCharacter character) noexcept;
    static float advance(float current,
                         float target,
                         float coefficient) noexcept;

    ModuleSpec moduleSpec;
    DriveSettings targetSettings;
    DriveCharacter characterTarget {DriveCharacter::warm};

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> delayedDry;
    std::vector<std::vector<float>> dryDelay;
    std::vector<float> toneState;

    float currentAmount {};
    float currentTone {};
    float currentMix {0.7f};
    float currentTrimDb {};
    float currentDryEnergy {};
    float currentWetEnergy {};
    float currentMatchGain {1.0f};
    float targetMatchGain {1.0f};
    float currentLevelMatchWeight {1.0f};
    float currentWetEnable {};
    std::array<float, 3> characterWeights {0.0f, 1.0f, 0.0f};
    std::array<float, 3> characterWeightSteps {};

    float parameterCoefficient {};
    float baseParameterCoefficient {};
    float matchCoefficient {};
    float toneCoefficient {};
    int dryWritePosition {};
    int characterFadeSamplesRemaining {};
    int latency {};
    bool prepared {};
};
}
