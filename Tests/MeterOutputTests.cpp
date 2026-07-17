#include <JuceHeader.h>

#include "../Source/DSP/Metering.h"
#include "../Source/DSP/OutputSafety.h"

namespace
{
constexpr double testSampleRate = 48000.0;

void fillSine(juce::AudioBuffer<float>& buffer,
              double& phase,
              float amplitude,
              double frequency)
{
    const auto phaseDelta = juce::MathConstants<double>::twoPi
                            * frequency / testSampleRate;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = amplitude * static_cast<float>(std::sin(phase));
        phase += phaseDelta;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

void processConstant(Voxline::Dsp::BallisticMeter& meter,
                     int preferredBlockSize,
                     int totalSamples,
                     float value)
{
    juce::AudioBuffer<float> buffer(2, preferredBlockSize);
    auto remaining = totalSamples;

    while (remaining > 0)
    {
        const auto samplesThisTime = juce::jmin(remaining, preferredBlockSize);
        buffer.setSize(2, samplesThisTime, false, false, true);
        buffer.clear();

        if (value != 0.0f)
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                juce::FloatVectorOperations::fill(
                    buffer.getWritePointer(channel), value, samplesThisTime);

        meter.measureBlock(buffer);
        remaining -= samplesThisTime;
    }
}

class MeterOutputTests final : public juce::UnitTest
{
public:
    MeterOutputTests() : juce::UnitTest("Meter and output safety", "VOXLINE") {}

    void runTest() override
    {
        beginTest("settled sine reports peak and RMS in decibels");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 480, 2});

            juce::AudioBuffer<float> buffer(2, 480);
            Voxline::Dsp::MeterFrame frame;
            double phase = 0.0;

            for (int block = 0; block < 200; ++block)
            {
                fillSine(buffer, phase, 0.5f, 1000.0);
                frame = meter.measureBlock(buffer);
            }

            expectEquals(frame.channelCount, 2);
            for (const auto& channel : frame.channels)
            {
                expectWithinAbsoluteError(channel.peakDbfs, -6.0206f, 0.05f);
                expectWithinAbsoluteError(channel.rmsDbfs, -9.0309f, 0.05f);
            }
        }

        beginTest("silence stays at the meter floor without clip hold");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 512, 2});
            juce::AudioBuffer<float> silence(2, 512);
            silence.clear();

            const auto frame = meter.measureBlock(silence);
            expect(! frame.clipHeld);
            for (const auto& channel : frame.channels)
            {
                expectEquals(channel.peakDbfs,
                             Voxline::Dsp::silenceFloorDbfs);
                expectEquals(channel.rmsDbfs,
                             Voxline::Dsp::silenceFloorDbfs);
                expectEquals(channel.truePeakDbtp,
                             Voxline::Dsp::silenceFloorDbfs);
            }
        }

        beginTest("release timing is independent of block size");
        {
            Voxline::Dsp::BallisticMeter meter64;
            Voxline::Dsp::BallisticMeter meter512;
            meter64.prepare({testSampleRate, 64, 2});
            meter512.prepare({testSampleRate, 512, 2});

            processConstant(meter64, 64, 96000, 1.0f);
            processConstant(meter512, 512, 96000, 1.0f);
            processConstant(meter64, 64, 19200, 0.0f);
            processConstant(meter512, 512, 19200, 0.0f);

            juce::AudioBuffer<float> empty(2, 0);
            const auto frame64 = meter64.measureBlock(empty);
            const auto frame512 = meter512.measureBlock(empty);
            expect(std::abs(frame64.channels[0].peakDbfs
                            - frame512.channels[0].peakDbfs)
                   < 0.1f);
        }

        beginTest("inter-sample stress reports true peak above sample peak");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 256, 1});
            juce::AudioBuffer<float> buffer(1, 256);
            Voxline::Dsp::MeterFrame frame;
            constexpr std::array<float, 4> stress {-0.7f, 0.7f, 0.7f, -0.7f};

            for (int block = 0; block < 200; ++block)
            {
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample(0, sample,
                                     stress[static_cast<size_t>(sample) % stress.size()]);
                frame = meter.measureBlock(buffer);
            }

            expect(frame.channels[0].truePeakDbtp
                   > frame.channels[0].peakDbfs);
        }

        beginTest("sample clipping latches until explicitly cleared");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 16, 1});
            juce::AudioBuffer<float> buffer(1, 16);
            buffer.clear();
            buffer.setSample(0, 7, 1.0f);

            expect(meter.measureBlock(buffer).clipHeld);
            meter.clearClipHold();
            buffer.clear();
            expect(! meter.measureBlock(buffer).clipHeld);
        }

        beginTest("soft clip transfer is C1 continuous at the threshold");
        {
            constexpr float threshold = 0.98f;
            constexpr float epsilon = 1.0e-5f;
            constexpr float slopeStep = 1.0e-4f;
            const auto below =
                Voxline::Dsp::EmergencySoftClipper::transfer(threshold - epsilon);
            const auto above =
                Voxline::Dsp::EmergencySoftClipper::transfer(threshold + epsilon);
            expect(std::abs(above - below) < 5.0e-5f);

            const auto atThreshold =
                Voxline::Dsp::EmergencySoftClipper::transfer(threshold);
            const auto leftSlope =
                (atThreshold
                 - Voxline::Dsp::EmergencySoftClipper::transfer(
                     threshold - slopeStep))
                / slopeStep;
            const auto rightSlope =
                (Voxline::Dsp::EmergencySoftClipper::transfer(
                     threshold + slopeStep)
                 - atThreshold)
                / slopeStep;
            expect(std::abs(leftSlope - rightSlope) < 0.02f);
        }

        beginTest("extreme input remains finite and bounded");
        {
            Voxline::Dsp::EmergencySoftClipper clipper;
            juce::AudioBuffer<float> buffer(2, 2);
            buffer.setSample(0, 0, 100.0f);
            buffer.setSample(0, 1, -100.0f);
            buffer.setSample(1, 0, -100.0f);
            buffer.setSample(1, 1, 100.0f);

            clipper.process(buffer);
            expect(clipper.wasActive());

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    const auto value = buffer.getSample(channel, sample);
                    expect(std::isfinite(value));
                    expect(std::abs(value) <= 1.0f);
                }
        }
    }
};

MeterOutputTests meterOutputTests;
}
