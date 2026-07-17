#include <JuceHeader.h>

#include "../Source/DSP/VocalEq.h"

namespace
{
using namespace Voxline::Dsp;

VocalEqSettings flatSettings()
{
    VocalEqSettings settings;
    settings.hpf.enabled = false;
    settings.low.enabled = false;
    settings.mud.enabled = false;
    settings.presence.enabled = false;
    settings.air.enabled = false;
    settings.lpf.enabled = false;
    return settings;
}

void fillSine(juce::AudioBuffer<float>& buffer,
              double sampleRate,
              float frequencyHz,
              float amplitude,
              int sampleOffset = 0)
{
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample(channel, sample, amplitude * std::sin(
                juce::MathConstants<float>::twoPi * frequencyHz
                * static_cast<float>(sample + sampleOffset)
                / static_cast<float>(sampleRate)));
}

void settleTransition(VocalEq& eq, double sampleRate, int channels)
{
    juce::AudioBuffer<float> silence(channels,
        static_cast<int>(std::ceil(sampleRate * 0.012)));
    silence.clear();
    eq.process(silence);
}

float responseDb(VocalEq& eq, float frequencyHz)
{
    return juce::Decibels::gainToDecibels(
        eq.magnitudeAt(frequencyHz), -120.0f);
}

class VocalEqTests final : public juce::UnitTest
{
public:
    VocalEqTests() : juce::UnitTest("Vocal EQ", "VOXLINE") {}

    void runTest() override
    {
        beginTest("default settings are a true null");
        {
            VocalEq eq;
            eq.prepare({48000.0, 512, 2});

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            eq.process(buffer);

            for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
                    expectWithinAbsoluteError(buffer.getSample(channel, sample),
                                              original.getSample(channel, sample),
                                              1.0e-5f);
        }

        beginTest("global bypass is exact after its smoothing transition");
        {
            VocalEq eq;
            eq.prepare({48000.0, 512, 2});
            auto settings = VocalEqSettings{};
            settings.enabled = false;
            eq.setTargetSettings(settings);
            settleTransition(eq, 48000.0, 2);
            eq.reset();

            juce::AudioBuffer<float> buffer(2, 512);
            fillSine(buffer, 48000.0, 997.0f, 0.37f);
            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);
            eq.process(buffer);

            for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
                    expectEquals(buffer.getSample(channel, sample),
                                 original.getSample(channel, sample));
        }

        beginTest("tone bands reach their requested positive and negative gains");
        {
            struct BandCase
            {
                enum class Band { low, mud, presence, air } band;
                float frequencyHz;
                float measurementHz;
            };

            constexpr std::array cases {
                BandCase{BandCase::Band::low, 160.0f, 30.0f},
                BandCase{BandCase::Band::mud, 350.0f, 350.0f},
                BandCase{BandCase::Band::presence, 2500.0f, 2500.0f},
                BandCase{BandCase::Band::air, 10000.0f, 18000.0f}
            };

            for (const auto gainDb : {-12.0f, 12.0f})
            {
                for (const auto& testCase : cases)
                {
                    VocalEq eq;
                    eq.prepare({48000.0, 512, 2});
                    auto settings = flatSettings();
                    EqBandSettings* band = nullptr;
                    switch (testCase.band)
                    {
                        case BandCase::Band::low: band = &settings.low; break;
                        case BandCase::Band::mud: band = &settings.mud; break;
                        case BandCase::Band::presence: band = &settings.presence; break;
                        case BandCase::Band::air: band = &settings.air; break;
                    }
                    band->enabled = true;
                    band->frequencyHz = testCase.frequencyHz;
                    band->gainDb = gainDb;
                    eq.setTargetSettings(settings);

                    expectWithinAbsoluteError(responseDb(eq, testCase.measurementHz),
                                              gainDb, 0.75f);
                }
            }
        }

        beginTest("disabled cuts are null and enabled slopes attenuate monotonically");
        {
            VocalEq eq;
            eq.prepare({48000.0, 512, 2});
            auto settings = flatSettings();
            eq.setTargetSettings(settings);
            expectWithinAbsoluteError(eq.magnitudeAt(50.0f), 1.0f, 1.0e-6f);
            expectWithinAbsoluteError(eq.magnitudeAt(16000.0f), 1.0f, 1.0e-6f);

            std::array<float, 4> hpfDb {};
            std::array<float, 4> lpfDb {};
            constexpr std::array slopes {
                FilterSlope::db12, FilterSlope::db24,
                FilterSlope::db36, FilterSlope::db48
            };

            for (size_t i = 0; i < slopes.size(); ++i)
            {
                settings = flatSettings();
                settings.hpf = {true, 200.0f, 0.0f, 0.707f};
                settings.hpfSlope = slopes[i];
                eq.setTargetSettings(settings);
                hpfDb[i] = responseDb(eq, 50.0f);

                settings = flatSettings();
                settings.lpf = {true, 6000.0f, 0.0f, 0.707f};
                settings.lpfSlope = slopes[i];
                eq.setTargetSettings(settings);
                lpfDb[i] = responseDb(eq, 16000.0f);
            }

            for (size_t i = 1; i < hpfDb.size(); ++i)
            {
                expect(hpfDb[i] < hpfDb[i - 1]);
                expect(lpfDb[i] < lpfDb[i - 1]);
            }
        }

        beginTest("centres track sample rate and clamp below the safe Nyquist margin");
        {
            for (const auto sampleRate : {44100.0, 48000.0, 88200.0, 96000.0})
            {
                VocalEq eq;
                eq.prepare({sampleRate, 512, 2});
                auto settings = flatSettings();
                settings.presence = {true, 2500.0f, 12.0f, 1.0f};
                eq.setTargetSettings(settings);

                auto strongestHz = 0.0f;
                auto strongestDb = -120.0f;
                for (auto frequency = 1500.0f; frequency <= 3500.0f; frequency += 5.0f)
                {
                    const auto db = responseDb(eq, frequency);
                    if (db > strongestDb)
                    {
                        strongestDb = db;
                        strongestHz = frequency;
                    }
                }
                expectWithinAbsoluteError(strongestHz, 2500.0f, 50.0f);

                settings.presence.frequencyHz = 100000.0f;
                eq.setTargetSettings(settings);
                const auto expectedClamp = static_cast<float>(0.45 * sampleRate);
                strongestHz = 0.0f;
                strongestDb = -120.0f;
                for (auto frequency = expectedClamp * 0.90f;
                     frequency <= expectedClamp * 1.08f;
                     frequency += expectedClamp * 0.001f)
                {
                    const auto db = responseDb(eq, frequency);
                    if (db > strongestDb)
                    {
                        strongestDb = db;
                        strongestHz = frequency;
                    }
                }
                expectWithinAbsoluteError(strongestHz, expectedClamp,
                                          expectedClamp * 0.02f);
                expect(strongestHz <= static_cast<float>(0.46 * sampleRate));
            }
        }

        beginTest("automation crossfade remains finite and bounded");
        {
            VocalEq eq;
            eq.prepare({48000.0, 1024, 2});
            auto before = flatSettings();
            before.low = {true, 100.0f, -12.0f, 0.4f};
            before.mud = {true, 250.0f, -12.0f, 0.5f};
            before.presence = {true, 1200.0f, -12.0f, 0.5f};
            before.air = {true, 7000.0f, -12.0f, 0.4f};
            eq.setTargetSettings(before);
            settleTransition(eq, 48000.0, 2);

            auto after = before;
            after.low = {true, 250.0f, 12.0f, 2.0f};
            after.mud = {true, 700.0f, 12.0f, 3.0f};
            after.presence = {true, 5000.0f, 12.0f, 3.0f};
            after.air = {true, 16000.0f, 12.0f, 2.0f};
            eq.setTargetSettings(after);

            juce::AudioBuffer<float> buffer(2, 1024);
            fillSine(buffer, 48000.0, 997.0f, 0.2f);
            eq.process(buffer);

            auto maximumDelta = 0.0f;
            for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
                    expect(std::isfinite(buffer.getSample(channel, sample)));
                for (auto sample = 1; sample < buffer.getNumSamples(); ++sample)
                    maximumDelta = juce::jmax(maximumDelta,
                        std::abs(buffer.getSample(channel, sample)
                                 - buffer.getSample(channel, sample - 1)));
            }
            expect(maximumDelta < 0.5f);
        }
    }
};

VocalEqTests vocalEqTests;
}
