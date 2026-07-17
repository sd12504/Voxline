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

    struct DelayLine
    {
        void prepare(int samples);
        void reset() noexcept;
        StereoSample read(int delaySamples) const noexcept;
        void write(StereoSample sample) noexcept;

        std::vector<float> left;
        std::vector<float> right;
        int writePosition {};
    };

    struct Engine
    {
        void prepare(SpaceMode, const ModuleSpec&);
        void reset() noexcept;
        StereoSample process(StereoSample input,
                             const SpaceSettings&) noexcept;

        StereoSample processReverb(StereoSample,
                                   const SpaceSettings&) noexcept;
        StereoSample processSlap(StereoSample,
                                 const SpaceSettings&) noexcept;
        StereoSample processWidth(StereoSample,
                                  const SpaceSettings&) noexcept;

        SpaceMode mode {SpaceMode::room};
        double sampleRate {44100.0};
        DelayLine preDelay;
        std::array<DelayLine, 8> lines;
        int lineCount {};
        std::array<float, 2> diffusion {};
        std::array<float, 2> toneState {};
        float modulationPhase {};
    };

    static size_t modeIndex(SpaceMode) noexcept;
    static float advance(float current,
                         float target,
                         float coefficient) noexcept;
    Engine& engineFor(SpaceMode) noexcept;

    ModuleSpec moduleSpec;
    SpaceSettings targetSettings;
    SpaceSettings currentSettings;
    std::array<Engine, 5> engines;

    SpaceMode currentMode {SpaceMode::plate};
    SpaceMode nextMode {SpaceMode::plate};
    float modeFade {1.0f};
    float parameterCoefficient {};
    float duckAttackCoefficient {};
    float duckReleaseCoefficient {};
    float duckEnvelope {};
    bool prepared {};
    bool hasProcessed {};
};
}
