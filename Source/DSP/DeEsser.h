#pragma once

#include "DspTypes.h"

#include <array>

namespace Voxline::Dsp
{
enum class DeEssMode
{
    split,
    wide
};

enum class DeEssMonitor
{
    off,
    detector
};

struct DeEsserSettings
{
    float amount {};
    float focusHz {6500.0f};
    float sensitivityDb {-18.0f};
    float maxRangeDb {6.0f};
    DeEssMode mode {DeEssMode::split};
};

struct DeEssMetrics
{
    float reductionDb {};
    bool sActive {};
};

class DeEsser
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const DeEsserSettings&) noexcept;
    DeEssMetrics process(
        juce::AudioBuffer<float>&,
        DeEssMonitor monitor = DeEssMonitor::off) noexcept;

private:
    struct BiquadCoefficients
    {
        float b0 {};
        float b1 {};
        float b2 {};
        float a1 {};
        float a2 {};
    };

    struct BiquadState
    {
        float x1 {};
        float x2 {};
        float y1 {};
        float y2 {};
    };

    BiquadCoefficients makeDetectorCoefficients(float focusHz) const noexcept;
    float processDetectorSample(float input,
                                BiquadState& state) const noexcept;
    void beginAmountTransition(float target) noexcept;
    void beginFocusTransition(float targetHz) noexcept;
    void beginModeTransition(DeEssMode target) noexcept;
    void advanceControls() noexcept;
    void enterIdle() noexcept;
    void updateEnvelopeCoefficients() noexcept;

    DeEsserSettings settings;
    double sampleRate {44100.0};
    int preparedChannels {2};

    std::array<float, 2> lowpassState {};
    std::array<float, 2> highpassSamples {};
    std::array<float, 2> detectorSamples {};
    std::array<BiquadState, 2> detectorStates {};
    float detectorEnvelope {};
    float reductionEnvelopeDb {};

    BiquadCoefficients detectorCoefficients;
    BiquadCoefficients detectorTargetCoefficients;
    BiquadCoefficients detectorCoefficientStep;
    float focusCoefficient {};
    float targetFocusCoefficient {};
    float focusCoefficientStep {};
    int focusTransitionRemaining {};

    float currentAmount {};
    float targetAmount {};
    float amountStep {};
    int amountTransitionRemaining {};

    float currentModeMix {};
    float targetModeMix {};
    float modeMixStep {};
    int modeTransitionRemaining {};

    float detectorAttackCoefficient {};
    float detectorReleaseCoefficient {};
    float reductionAttackCoefficient {};
    float reductionReleaseCoefficient {};
    bool idle {true};
};
}
