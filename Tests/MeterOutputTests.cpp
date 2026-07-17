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

Voxline::Dsp::MeterFrame processConstant(
    Voxline::Dsp::BallisticMeter& meter,
    int preferredBlockSize,
    int totalSamples,
    float value)
{
    juce::AudioBuffer<float> buffer(2, preferredBlockSize);
    auto remaining = totalSamples;
    Voxline::Dsp::MeterFrame frame;

    while (remaining > 0)
    {
        const auto samplesThisTime = juce::jmin(remaining, preferredBlockSize);
        buffer.setSize(2, samplesThisTime, false, false, true);
        buffer.clear();

        if (value != 0.0f)
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                juce::FloatVectorOperations::fill(
                    buffer.getWritePointer(channel), value, samplesThisTime);

        frame = meter.measureBlock(buffer);
        remaining -= samplesThisTime;
    }

    return frame;
}

Voxline::Dsp::MeterFrame processQuarterRateSine(
    Voxline::Dsp::BallisticMeter& meter,
    int blockSize,
    int totalSamples,
    float amplitude)
{
    const auto sampledAmplitude =
        amplitude * std::sqrt(0.5f);
    constexpr std::array<float, 4> phases {1.0f, 1.0f, -1.0f, -1.0f};
    juce::AudioBuffer<float> buffer(1, blockSize);
    Voxline::Dsp::MeterFrame frame;
    auto offset = 0;

    while (offset < totalSamples)
    {
        const auto samplesThisTime =
            juce::jmin(blockSize, totalSamples - offset);
        buffer.setSize(1, samplesThisTime, false, false, true);

        for (int sample = 0; sample < samplesThisTime; ++sample)
            buffer.setSample(
                0, sample,
                sampledAmplitude
                    * phases[static_cast<size_t>(offset + sample)
                             % phases.size()]);

        frame = meter.measureBlock(buffer);
        offset += samplesThisTime;
    }

    return frame;
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

        beginTest("band-limited true peak reconstructs a quarter-rate sine");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 256, 1});
            const auto frame =
                processQuarterRateSine(meter, 256, 96000, 0.9f);
            const auto expectedDbtp =
                Voxline::Dsp::gainToDbfs(0.9f);

            expectWithinAbsoluteError(frame.channels[0].truePeakDbtp,
                                      expectedDbtp, 0.1f);
        }

        beginTest("true peak is invariant to cross-block partitioning");
        {
            Voxline::Dsp::BallisticMeter meter2;
            Voxline::Dsp::BallisticMeter meter512;
            meter2.prepare({testSampleRate, 512, 1});
            meter512.prepare({testSampleRate, 512, 1});

            const auto frame2 =
                processQuarterRateSine(meter2, 2, 96000, 0.9f);
            const auto frame512 =
                processQuarterRateSine(meter512, 512, 96000, 0.9f);

            expectWithinAbsoluteError(frame2.channels[0].truePeakDbtp,
                                      frame512.channels[0].truePeakDbtp,
                                      0.05f);
        }

        beginTest("peak and RMS attacks use their specified time constants");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 512, 2});
            const auto expectedAfterOneTimeConstant =
                Voxline::Dsp::gainToDbfs(1.0f - std::exp(-1.0f));

            const auto peakFrame =
                processConstant(meter, 128, 384, 1.0f);
            expectWithinAbsoluteError(peakFrame.channels[0].peakDbfs,
                                      expectedAfterOneTimeConstant, 0.08f);

            meter.reset();
            const auto rmsFrame =
                processConstant(meter, 512, 3840, 1.0f);
            expectWithinAbsoluteError(rmsFrame.channels[0].rmsDbfs,
                                      expectedAfterOneTimeConstant, 0.08f);
        }

        beginTest("peak and RMS releases use their specified time constants");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 512, 2});
            processConstant(meter, 512, 144000, 1.0f);

            const auto peakFrame =
                processConstant(meter, 512, 19200, 0.0f);
            expectWithinAbsoluteError(peakFrame.channels[0].peakDbfs,
                                      -8.6859f, 0.08f);

            meter.reset();
            processConstant(meter, 512, 144000, 1.0f);
            const auto rmsFrame =
                processConstant(meter, 512, 28800, 0.0f);
            expectWithinAbsoluteError(rmsFrame.channels[0].rmsDbfs,
                                      -8.6859f, 0.08f);
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

        beginTest("clip clear is an audio-thread request");
        {
            Voxline::Dsp::BallisticMeter meter;
            meter.prepare({testSampleRate, 1024, 1});
            juce::AudioBuffer<float> clipped(1, 1024);
            juce::FloatVectorOperations::fill(clipped.getWritePointer(0),
                                              1.0f,
                                              clipped.getNumSamples());

            std::atomic<bool> keepClearing {true};
            std::atomic<bool> clearerStarted {false};
            std::thread clearer(
                [&]
                {
                    clearerStarted.store(true, std::memory_order_release);
                    while (keepClearing.load(std::memory_order_acquire))
                        meter.clearClipHold();
                });

            while (! clearerStarted.load(std::memory_order_acquire))
                std::this_thread::yield();

            auto falseFrames = 0;
            for (int block = 0; block < 5000; ++block)
                if (! meter.measureBlock(clipped).clipHeld)
                    ++falseFrames;

            keepClearing.store(false, std::memory_order_release);
            clearer.join();
            expectEquals(falseFrames, 0);
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

        beginTest("output safety publishes activity after each complete block");
        {
            Voxline::Dsp::EmergencySoftClipper clipper;
            juce::AudioBuffer<float> clipped(1, 1);
            clipped.setSample(0, 0, 2.0f);
            clipper.process(clipped);
            expect(clipper.wasActive());

            juce::AudioBuffer<float> longBlock(1, 16 * 1024 * 1024);
            longBlock.clear();
            longBlock.setSample(0, longBlock.getNumSamples() - 1, 2.0f);
            std::atomic<bool> started {false};
            std::atomic<bool> finished {false};

            std::thread processor(
                [&]
                {
                    started.store(true, std::memory_order_release);
                    clipper.process(longBlock);
                    finished.store(true, std::memory_order_release);
                });

            while (! started.load(std::memory_order_acquire))
                std::this_thread::yield();

            auto changedBeforeBlockFinished = false;
            while (! finished.load(std::memory_order_acquire))
                changedBeforeBlockFinished =
                    changedBeforeBlockFinished || ! clipper.wasActive();

            processor.join();
            expect(! changedBeforeBlockFinished);
            expect(clipper.wasActive());

            juce::AudioBuffer<float> silence(1, 1);
            silence.clear();
            clipper.process(silence);
            expect(! clipper.wasActive());
        }
    }
};

MeterOutputTests meterOutputTests;
}
