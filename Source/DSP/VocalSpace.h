#pragma once

#include "DspTypes.h"

#include <array>
#include <vector>

namespace Voxline::Dsp
{
enum class SpaceMode
{
    room,
    plate,
    hall,
    slap,
    width
};

struct SpaceSettings
{
    float amount {};
    SpaceMode mode {SpaceMode::plate};
    float preDelayMs {28.0f};
    float sizeOrTime {0.62f};
    float decaySeconds {1.6f};
    float tone {0.12f};
    float width {1.0f};
    float ducking {0.42f};
    float feedback {0.2f};
    bool monoSafety {true};
};

class VocalSpace
{
public:
    void prepare(const ModuleSpec&);
    void reset() noexcept;
    void setTargetSettings(const SpaceSettings&) noexcept;
    void process(juce::AudioBuffer<float>& audio,
                 const juce::AudioBuffer<float>& drySidechain) noexcept;
    double tailSeconds() const noexcept;

private:
    struct StereoSample
    {
        float left {};
        float right {};
    };

    struct MonoDelay
    {
        void prepare(int maximumSamples);
        void reset() noexcept;
        float read(float delaySamples) const noexcept;
        void write(float sample) noexcept;

        std::vector<float> storage;
        int writePosition {};
    };

    struct CrossfadedTap
    {
        void prepare(double sampleRate) noexcept;
        void reset() noexcept;
        float read(const MonoDelay& delay,
                   float requestedDelaySamples,
                   float modulationSamples = 0.0f,
                   float directSample = 0.0f) noexcept;

        float sampleAt(const MonoDelay& delay,
                       float delaySamples,
                       float directSample) const noexcept;

        float fromDelaySamples {};
        float toDelaySamples {};
        float queuedDelaySamples {};
        float fade {1.0f};
        float fadeStep {1.0f};
        bool initialised {};
        bool hasQueuedTarget {};
    };

    struct AllPass
    {
        void prepare(double sampleRate, int maximumSamples);
        void reset() noexcept;
        float process(float input,
                      float delaySamples,
                      float modulationSamples,
                      float feedback) noexcept;

        MonoDelay delay;
        CrossfadedTap tap;
    };

    struct Engine
    {
        void prepare(SpaceMode, const ModuleSpec&);
        void reset() noexcept;
        StereoSample process(StereoSample,
                             const SpaceSettings&) noexcept;

        StereoSample processRoom(StereoSample,
                                 const SpaceSettings&) noexcept;
        StereoSample processPlate(StereoSample,
                                  const SpaceSettings&) noexcept;
        StereoSample processHall(StereoSample,
                                 const SpaceSettings&) noexcept;
        StereoSample processSlap(StereoSample,
                                 const SpaceSettings&) noexcept;
        StereoSample processWidth(StereoSample,
                                  const SpaceSettings&) noexcept;
        StereoSample processPreDelay(StereoSample,
                                     float preDelayMs) noexcept;
        StereoSample processFdn(StereoSample input,
                                const SpaceSettings& settings,
                                const float* delaySeconds,
                                int activeLines,
                                float decayScale,
                                float modulationDepthMs,
                                float inputGain) noexcept;

        SpaceMode mode {SpaceMode::room};
        double sampleRate {44100.0};
        MonoDelay preDelayLeft;
        MonoDelay preDelayRight;
        CrossfadedTap preDelayTapLeft;
        CrossfadedTap preDelayTapRight;
        std::array<MonoDelay, 8> tank;
        std::array<CrossfadedTap, 8> tankTaps;
        std::array<AllPass, 8> diffusers;
        std::array<float, 8> dampingState {};
        std::array<float, 8> modulationPhase {};
        std::array<float, 2> toneState {};
        int lineCount {};
    };

    static size_t modeIndex(SpaceMode) noexcept;
    static SpaceMode sanitiseMode(SpaceMode) noexcept;
    static float advance(float current,
                         float target,
                         float coefficient) noexcept;
    Engine& engineFor(SpaceMode) noexcept;
    void beginModeTransitionIfNeeded() noexcept;

    ModuleSpec moduleSpec;
    SpaceSettings targetSettings;
    SpaceSettings currentSettings;
    std::array<Engine, 5> engines;

    SpaceMode currentMode {SpaceMode::plate};
    SpaceMode nextMode {SpaceMode::plate};
    float modeFade {1.0f};
    float modeFadeStep {1.0f};
    float parameterCoefficient {};
    float duckAttackCoefficient {};
    float duckReleaseCoefficient {};
    float duckEnvelope {};
    bool prepared {};
    bool hasProcessed {};
};
}
