#include "VocalEq.h"

#include <array>
#include <complex>
#include <limits>

namespace
{
using namespace Voxline::Dsp;

constexpr int maximumChannels = 2;
constexpr int maximumSlopeStages = 4;
constexpr int toneBandCount = 4;

int slopeStages(FilterSlope slope) noexcept
{
    return static_cast<int>(slope) + 1;
}

EqBandSettings clampBand(EqBandSettings band, double sampleRate) noexcept
{
    band.frequencyHz = juce::jlimit(
        20.0f, static_cast<float>(sampleRate * 0.45), band.frequencyHz);
    band.gainDb = juce::jlimit(-12.0f, 12.0f, band.gainDb);
    band.q = juce::jlimit(0.1f, 10.0f, band.q);
    return band;
}

VocalEqSettings clampSettings(VocalEqSettings settings,
                               double sampleRate) noexcept
{
    settings.hpf = clampBand(settings.hpf, sampleRate);
    settings.low = clampBand(settings.low, sampleRate);
    settings.mud = clampBand(settings.mud, sampleRate);
    settings.presence = clampBand(settings.presence, sampleRate);
    settings.air = clampBand(settings.air, sampleRate);
    settings.lpf = clampBand(settings.lpf, sampleRate);
    return settings;
}

bool bandsEqual(const EqBandSettings& lhs,
                const EqBandSettings& rhs) noexcept
{
    return lhs.enabled == rhs.enabled
        && juce::exactlyEqual(lhs.frequencyHz, rhs.frequencyHz)
        && juce::exactlyEqual(lhs.gainDb, rhs.gainDb)
        && juce::exactlyEqual(lhs.q, rhs.q);
}

bool settingsEqual(const VocalEqSettings& lhs,
                   const VocalEqSettings& rhs) noexcept
{
    return lhs.enabled == rhs.enabled
        && bandsEqual(lhs.hpf, rhs.hpf)
        && bandsEqual(lhs.low, rhs.low)
        && bandsEqual(lhs.mud, rhs.mud)
        && bandsEqual(lhs.presence, rhs.presence)
        && bandsEqual(lhs.air, rhs.air)
        && bandsEqual(lhs.lpf, rhs.lpf)
        && lhs.hpfSlope == rhs.hpfSlope
        && lhs.lpfSlope == rhs.lpfSlope;
}

juce::IIRCoefficients makeHighPass(double sampleRate,
                                   const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makeHighPass(
        sampleRate, band.frequencyHz, band.q);
}

juce::IIRCoefficients makeLowPass(double sampleRate,
                                  const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makeLowPass(
        sampleRate, band.frequencyHz, band.q);
}

juce::IIRCoefficients makeLowShelf(double sampleRate,
                                   const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makeLowShelf(
        sampleRate, band.frequencyHz, band.q,
        juce::Decibels::decibelsToGain(band.gainDb));
}

juce::IIRCoefficients makePeak(double sampleRate,
                               const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makePeakFilter(
        sampleRate, band.frequencyHz, band.q,
        juce::Decibels::decibelsToGain(band.gainDb));
}

juce::IIRCoefficients makeHighShelf(double sampleRate,
                                    const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makeHighShelf(
        sampleRate, band.frequencyHz, band.q,
        juce::Decibels::decibelsToGain(band.gainDb));
}

juce::IIRCoefficients makeBandPass(double sampleRate,
                                   const EqBandSettings& band) noexcept
{
    return juce::IIRCoefficients::makeBandPass(
        sampleRate, band.frequencyHz, band.q);
}

size_t toneBandIndex(VocalEqBand band) noexcept
{
    switch (band)
    {
        case VocalEqBand::low: return 0;
        case VocalEqBand::mud: return 1;
        case VocalEqBand::presence: return 2;
        case VocalEqBand::air: return 3;
        case VocalEqBand::none:
        case VocalEqBand::hpf:
        case VocalEqBand::lpf:
            break;
    }
    return 0;
}

float coefficientMagnitude(const juce::IIRCoefficients& coefficients,
                           double sampleRate,
                           float frequencyHz) noexcept
{
    const auto angle = -juce::MathConstants<double>::twoPi
        * static_cast<double>(frequencyHz) / sampleRate;
    const auto z = std::polar(1.0, angle);
    const auto z2 = z * z;
    const auto* c = coefficients.coefficients;
    const auto numerator = static_cast<double>(c[0])
        + static_cast<double>(c[1]) * z
        + static_cast<double>(c[2]) * z2;
    const auto denominator = 1.0
        + static_cast<double>(c[3]) * z
        + static_cast<double>(c[4]) * z2;
    const auto denominatorMagnitude = std::abs(denominator);
    if (denominatorMagnitude <= std::numeric_limits<double>::epsilon())
        return 0.0f;
    return static_cast<float>(std::abs(numerator) / denominatorMagnitude);
}

struct ChannelChain
{
    std::array<juce::SingleThreadedIIRFilter, maximumSlopeStages> hpf;
    juce::SingleThreadedIIRFilter low;
    juce::SingleThreadedIIRFilter mud;
    juce::SingleThreadedIIRFilter presence;
    juce::SingleThreadedIIRFilter air;
    std::array<juce::SingleThreadedIIRFilter, maximumSlopeStages> lpf;
    int hpfStages {};
    int lpfStages {};
    bool lowEnabled {};
    bool mudEnabled {};
    bool presenceEnabled {};
    bool airEnabled {};
    std::array<juce::SingleThreadedIIRFilter, maximumSlopeStages> soloHpf;
    std::array<juce::SingleThreadedIIRFilter, toneBandCount> soloTone;
    std::array<juce::SingleThreadedIIRFilter, maximumSlopeStages> soloLpf;
    int soloHpfStages {};
    int soloLpfStages {};

    void reset() noexcept
    {
        for (auto& filter : hpf)
            filter.reset();
        low.reset();
        mud.reset();
        presence.reset();
        air.reset();
        for (auto& filter : lpf)
            filter.reset();
        for (auto& filter : soloHpf)
            filter.reset();
        for (auto& filter : soloTone)
            filter.reset();
        for (auto& filter : soloLpf)
            filter.reset();
    }

    void resetSolo() noexcept
    {
        for (auto& filter : soloHpf)
            filter.reset();
        for (auto& filter : soloTone)
            filter.reset();
        for (auto& filter : soloLpf)
            filter.reset();
    }

    float process(float sample) noexcept
    {
        for (auto stage = 0; stage < hpfStages; ++stage)
            sample = hpf[static_cast<size_t>(stage)].processSingleSampleRaw(sample);
        if (lowEnabled)
            sample = low.processSingleSampleRaw(sample);
        if (mudEnabled)
            sample = mud.processSingleSampleRaw(sample);
        if (presenceEnabled)
            sample = presence.processSingleSampleRaw(sample);
        if (airEnabled)
            sample = air.processSingleSampleRaw(sample);
        for (auto stage = 0; stage < lpfStages; ++stage)
            sample = lpf[static_cast<size_t>(stage)].processSingleSampleRaw(sample);
        return sample;
    }

    float processSolo(VocalEqBand band, float sample) noexcept
    {
        if (band == VocalEqBand::hpf)
        {
            auto filtered = sample;
            for (auto stage = 0; stage < soloHpfStages; ++stage)
                filtered = soloHpf[static_cast<size_t>(stage)]
                    .processSingleSampleRaw(filtered);
            return sample - filtered;
        }

        if (band == VocalEqBand::lpf)
        {
            auto filtered = sample;
            for (auto stage = 0; stage < soloLpfStages; ++stage)
                filtered = soloLpf[static_cast<size_t>(stage)]
                    .processSingleSampleRaw(filtered);
            return sample - filtered;
        }

        return soloTone[toneBandIndex(band)].processSingleSampleRaw(sample);
    }
};

void configureFilter(juce::SingleThreadedIIRFilter& filter,
                     bool enabled,
                     const juce::IIRCoefficients& coefficients) noexcept
{
    filter.reset();
    if (enabled)
        filter.setCoefficients(coefficients);
    else
        filter.makeInactive();
}

void configureChain(ChannelChain& chain,
                    const VocalEqSettings& settings,
                    double sampleRate) noexcept
{
    const auto hpfCoefficients = makeHighPass(sampleRate, settings.hpf);
    const auto hpfStageCount = slopeStages(settings.hpfSlope);
    chain.hpfStages = settings.hpf.enabled ? hpfStageCount : 0;
    for (auto stage = 0; stage < maximumSlopeStages; ++stage)
        configureFilter(chain.hpf[static_cast<size_t>(stage)],
                        settings.hpf.enabled && stage < hpfStageCount,
                        hpfCoefficients);

    chain.lowEnabled = settings.low.enabled;
    chain.mudEnabled = settings.mud.enabled;
    chain.presenceEnabled = settings.presence.enabled;
    chain.airEnabled = settings.air.enabled;
    configureFilter(chain.low, settings.low.enabled,
                    makeLowShelf(sampleRate, settings.low));
    configureFilter(chain.mud, settings.mud.enabled,
                    makePeak(sampleRate, settings.mud));
    configureFilter(chain.presence, settings.presence.enabled,
                    makePeak(sampleRate, settings.presence));
    configureFilter(chain.air, settings.air.enabled,
                    makeHighShelf(sampleRate, settings.air));

    const auto lpfCoefficients = makeLowPass(sampleRate, settings.lpf);
    const auto lpfStageCount = slopeStages(settings.lpfSlope);
    chain.lpfStages = settings.lpf.enabled ? lpfStageCount : 0;
    for (auto stage = 0; stage < maximumSlopeStages; ++stage)
        configureFilter(chain.lpf[static_cast<size_t>(stage)],
                        settings.lpf.enabled && stage < lpfStageCount,
                        lpfCoefficients);

    chain.soloHpfStages = hpfStageCount;
    chain.soloLpfStages = lpfStageCount;
    for (auto stage = 0; stage < maximumSlopeStages; ++stage)
    {
        configureFilter(chain.soloHpf[static_cast<size_t>(stage)], true,
                        hpfCoefficients);
        configureFilter(chain.soloLpf[static_cast<size_t>(stage)], true,
                        lpfCoefficients);
    }

    const std::array toneBands {
        settings.low, settings.mud, settings.presence, settings.air
    };
    for (size_t index = 0; index < toneBands.size(); ++index)
        configureFilter(chain.soloTone[index], true,
                        makeBandPass(sampleRate, toneBands[index]));
}

struct FilterBank
{
    VocalEqSettings settings;
    std::array<ChannelChain, maximumChannels> channels;

    float process(int channel, float sample, VocalEqBand soloBand) noexcept
    {
        if (soloBand != VocalEqBand::none)
            return channels[static_cast<size_t>(channel)]
                .processSolo(soloBand, sample);
        if (! settings.enabled)
            return sample;
        return channels[static_cast<size_t>(channel)].process(sample);
    }
};

void configureBank(FilterBank& bank,
                   const VocalEqSettings& settings,
                   double sampleRate) noexcept
{
    bank.settings = settings;
    for (auto& chain : bank.channels)
        configureChain(chain, settings, sampleRate);
}

float settingsMagnitude(const VocalEqSettings& settings,
                        double sampleRate,
                        float frequencyHz) noexcept
{
    if (! settings.enabled)
        return 1.0f;

    const auto evaluationFrequency = juce::jlimit(
        0.0f, static_cast<float>(sampleRate * 0.5), frequencyHz);
    auto magnitude = 1.0f;

    if (settings.hpf.enabled)
    {
        const auto stageMagnitude = coefficientMagnitude(
            makeHighPass(sampleRate, settings.hpf),
            sampleRate, evaluationFrequency);
        magnitude *= std::pow(stageMagnitude,
                              static_cast<float>(slopeStages(settings.hpfSlope)));
    }
    if (settings.low.enabled)
        magnitude *= coefficientMagnitude(makeLowShelf(sampleRate, settings.low),
                                          sampleRate, evaluationFrequency);
    if (settings.mud.enabled)
        magnitude *= coefficientMagnitude(makePeak(sampleRate, settings.mud),
                                          sampleRate, evaluationFrequency);
    if (settings.presence.enabled)
        magnitude *= coefficientMagnitude(makePeak(sampleRate, settings.presence),
                                          sampleRate, evaluationFrequency);
    if (settings.air.enabled)
        magnitude *= coefficientMagnitude(makeHighShelf(sampleRate, settings.air),
                                          sampleRate, evaluationFrequency);
    if (settings.lpf.enabled)
    {
        const auto stageMagnitude = coefficientMagnitude(
            makeLowPass(sampleRate, settings.lpf),
            sampleRate, evaluationFrequency);
        magnitude *= std::pow(stageMagnitude,
                              static_cast<float>(slopeStages(settings.lpfSlope)));
    }

    return magnitude;
}
} // namespace

namespace Voxline::Dsp
{
struct VocalEq::Impl
{
    double sampleRate {44100.0};
    int channels {2};
    int crossfadeSamples {441};
    int crossfadePosition {441};
    int activeBank {};
    int targetBank {1};
    bool crossfading {};
    bool hasPendingSettings {};
    VocalEqBand soloBand {VocalEqBand::none};
    VocalEqSettings targetSettings;
    VocalEqSettings pendingSettings;
    std::array<FilterBank, 2> banks;

    void beginTransition(const VocalEqSettings& settings) noexcept
    {
        targetBank = 1 - activeBank;
        configureBank(banks[static_cast<size_t>(targetBank)],
                      settings, sampleRate);
        crossfadePosition = 0;
        crossfading = true;
    }
};

VocalEq::VocalEq() = default;
VocalEq::~VocalEq() = default;

void VocalEq::prepare(const ModuleSpec& spec)
{
    if (impl == nullptr)
        impl = std::make_unique<Impl>();

    impl->sampleRate = juce::jmax(1.0, spec.sampleRate);
    impl->channels = juce::jlimit(1, maximumChannels, spec.channels);
    impl->crossfadeSamples = juce::jmax(
        1, static_cast<int>(std::round(impl->sampleRate * 0.010)));
    impl->crossfadePosition = impl->crossfadeSamples;
    impl->activeBank = 0;
    impl->targetBank = 1;
    impl->crossfading = false;
    impl->hasPendingSettings = false;
    impl->soloBand = VocalEqBand::none;
    impl->targetSettings = clampSettings(VocalEqSettings{}, impl->sampleRate);
    configureBank(impl->banks[0], impl->targetSettings, impl->sampleRate);
    configureBank(impl->banks[1], impl->targetSettings, impl->sampleRate);
}

void VocalEq::reset() noexcept
{
    if (impl == nullptr)
        return;
    for (auto& bank : impl->banks)
        for (auto& chain : bank.channels)
            chain.reset();
}

void VocalEq::setTargetSettings(const VocalEqSettings& settings) noexcept
{
    if (impl == nullptr)
        return;

    const auto clamped = clampSettings(settings, impl->sampleRate);
    if (settingsEqual(clamped, impl->targetSettings))
        return;

    impl->targetSettings = clamped;
    if (impl->crossfading)
    {
        impl->pendingSettings = clamped;
        impl->hasPendingSettings = true;
        return;
    }

    if (! settingsEqual(
            impl->banks[static_cast<size_t>(impl->activeBank)].settings,
            clamped))
        impl->beginTransition(clamped);
}

void VocalEq::setSoloBand(VocalEqBand band) noexcept
{
    if (impl == nullptr || impl->soloBand == band)
        return;

    impl->soloBand = band;
    for (auto& bank : impl->banks)
        for (auto& chain : bank.channels)
            chain.resetSolo();
}

void VocalEq::process(juce::AudioBuffer<float>& buffer) noexcept
{
    if (impl == nullptr)
        return;

    const auto channelsToProcess = juce::jmin(
        impl->channels, buffer.getNumChannels());

    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto mix = impl->crossfading
            ? juce::jlimit(0.0f, 1.0f,
                static_cast<float>(impl->crossfadePosition + 1)
                    / static_cast<float>(impl->crossfadeSamples))
            : 0.0f;

        for (auto channel = 0; channel < channelsToProcess; ++channel)
        {
            const auto input = buffer.getSample(channel, sample);
            const auto current = impl->banks[
                static_cast<size_t>(impl->activeBank)]
                    .process(channel, input, impl->soloBand);
            if (impl->crossfading)
            {
                const auto target = impl->banks[
                    static_cast<size_t>(impl->targetBank)]
                        .process(channel, input, impl->soloBand);
                buffer.setSample(channel, sample,
                                 current + mix * (target - current));
            }
            else
            {
                buffer.setSample(channel, sample, current);
            }
        }

        if (impl->crossfading
            && ++impl->crossfadePosition >= impl->crossfadeSamples)
        {
            impl->activeBank = impl->targetBank;
            impl->crossfading = false;
            if (impl->hasPendingSettings)
            {
                const auto pending = impl->pendingSettings;
                impl->hasPendingSettings = false;
                if (! settingsEqual(
                        impl->banks[static_cast<size_t>(impl->activeBank)].settings,
                        pending))
                    impl->beginTransition(pending);
            }
        }
    }
}

float VocalEq::magnitudeAt(float frequencyHz) const noexcept
{
    if (impl == nullptr)
        return 1.0f;
    return settingsMagnitude(impl->targetSettings,
                             impl->sampleRate, frequencyHz);
}
} // namespace Voxline::Dsp
