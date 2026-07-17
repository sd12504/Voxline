#include <JuceHeader.h>

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include "../Source/UI/LayoutLoader.h"

namespace
{
void fillTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto t = static_cast<float>(sample / sampleRate);
            const auto low = 0.22f * std::sin(juce::MathConstants<float>::twoPi * 180.0f * t);
            const auto presence = 0.15f * std::sin(juce::MathConstants<float>::twoPi * 3200.0f * t);
            const auto air = 0.08f * std::sin(juce::MathConstants<float>::twoPi * 10000.0f * t);
            buffer.setSample(channel, sample, low + presence + air);
        }
    }
}

float averageAbsoluteDifference(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    jassert(a.getNumChannels() == b.getNumChannels());
    jassert(a.getNumSamples() == b.getNumSamples());

    auto total = 0.0f;
    auto count = 0;

    for (int channel = 0; channel < a.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < a.getNumSamples(); ++sample)
        {
            total += std::abs(a.getSample(channel, sample) - b.getSample(channel, sample));
            ++count;
        }
    }

    return count > 0 ? total / static_cast<float>(count) : 0.0f;
}

void setFloatParameter(VoxlineAudioProcessor& processor, const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = dynamic_cast<juce::AudioParameterFloat*>(processor.getAPVTS().getParameter(parameterID)))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
}

void setBoolParameter(VoxlineAudioProcessor& processor, const juce::String& parameterID, bool enabled)
{
    if (auto* parameter = dynamic_cast<juce::AudioParameterBool*>(processor.getAPVTS().getParameter(parameterID)))
        parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
}

class VoxlineTests final : public juce::UnitTest
{
public:
    VoxlineTests()
        : juce::UnitTest("VOXLINE phases 1 to 5", "VOXLINE")
    {
    }

    void runTest() override
    {
        beginTest("processor exposes the full parameter set");

        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 51);

            auto* body = processor.getAPVTS().getParameter(VoxlineParameterIDs::body);
            expect(body != nullptr);
            expectWithinAbsoluteError(body->getNormalisableRange().start, -6.0f, 0.001f);
            expectWithinAbsoluteError(body->getNormalisableRange().end, 6.0f, 0.001f);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::compThreshold) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::deEssFreq) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::driveCharacter) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::spaceTime) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::spaceDucking) != nullptr);
            expectEquals(processor.getNumPrograms(), 9);
            expectEquals(processor.getProgramName(0), juce::String("Clean"));
            expect(processor.getTailLengthSeconds() >= 2.5);
        }

        beginTest("processor state round-trips parameter values");

        {
            VoxlineAudioProcessor sourceProcessor;
            expectEquals(sourceProcessor.getParameters().size(), 51);

            auto* firstParam = sourceProcessor.getParameters()[0];
            firstParam->setValueNotifyingHost(1.0f);
            setFloatParameter(sourceProcessor, VoxlineParameterIDs::compThreshold, -31.0f);

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            expect(state.getSize() > 0);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto* restoredFirstParam = restoredProcessor.getParameters()[0];
            expectWithinAbsoluteError(restoredFirstParam->getValue(), 1.0f, 0.001f);
            expectWithinAbsoluteError(restoredProcessor.getAPVTS().getRawParameterValue(
                                          VoxlineParameterIDs::compThreshold)->load(),
                                      -31.0f, 0.01f);
        }

        beginTest("editor uses the JSON layout and keeps real controls");

        {
            LayoutLoader layout;
            expect(layout.loadFromMemory(BinaryData::layout_json, BinaryData::layout_jsonSize));
            expectEquals(layout.getEditorWidth(), 1080);
            expectEquals(layout.getEditorHeight(), 720);
            expect(layout.hasKey("polishKnob"));
            expect(layout.hasKey("listenButton"));
            expect(layout.hasKey("outMeter"));

            VoxlineAudioProcessor processor;
            VoxlineAudioProcessorEditor editor(processor);

            expectEquals(editor.getWidth(), layout.getEditorWidth());
            expectEquals(editor.getHeight(), 940);

            int sliderCount = 0;
            int buttonCount = 0;
            int meterCount = 0;
            bool foundPolishSlider = false;
            bool foundListenButton = false;
            bool foundOutputMeter = false;

            for (int i = 0; i < editor.getNumChildComponents(); ++i)
            {
                const auto* child = editor.getChildComponent(i);

                if (const auto* slider = dynamic_cast<const juce::Slider*>(child))
                {
                    ++sliderCount;
                    if (slider->getBounds() == juce::Rectangle<int>{466, 116, 148, 148})
                        foundPolishSlider = true;
                }

                if (const auto* button = dynamic_cast<const juce::Button*>(child))
                {
                    ++buttonCount;
                    if (button->getBounds() == juce::Rectangle<int>{783, 21, 96, 34})
                        foundListenButton = true;
                }

                if (const auto* meter = dynamic_cast<const VoxlineLevelMeter*>(child))
                {
                    ++meterCount;
                    if (meter->getBounds() == layout.getBounds("outMeter"))
                        foundOutputMeter = true;
                }
            }

            expectGreaterOrEqual(sliderCount, 9);
            expectGreaterOrEqual(buttonCount, 11);
            expectEquals(meterCount, 2);
            expect(foundPolishSlider);
            expect(foundListenButton);
            expect(foundOutputMeter);

            editor.setAdvancedOpen(true);
            expectEquals(editor.getHeight(), 940);
            editor.setAdvancedOpen(false);
            expectEquals(editor.getHeight(), 720);
        }

        beginTest("Phase 3 default DSP audibly changes a vocal-like signal");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            fillTestSignal(buffer, 48000.0);
            juce::AudioBuffer<float> dryBuffer;
            dryBuffer.makeCopyOf(buffer);
            juce::MidiBuffer midi;

            processor.processBlock(buffer, midi);

            expect(averageAbsoluteDifference(buffer, dryBuffer) > 0.01f);
        }

        beginTest("Phase 3 bypass returns the dry signal");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            setBoolParameter(processor, VoxlineParameterIDs::bypass, true);

            juce::AudioBuffer<float> buffer(2, 512);
            fillTestSignal(buffer, 48000.0);
            juce::AudioBuffer<float> dryBuffer;
            dryBuffer.makeCopyOf(buffer);
            juce::MidiBuffer midi;

            juce::AudioBuffer<float> warmupBuffer;
            warmupBuffer.makeCopyOf(buffer);
            processor.processBlock(warmupBuffer, midi);
            processor.processBlock(buffer, midi);

            expectWithinAbsoluteError(averageAbsoluteDifference(buffer, dryBuffer), 0.0f, 1.0e-6f);
        }

        beginTest("Phase 3 output protection contains extreme drive levels");

        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            setFloatParameter(processor, VoxlineParameterIDs::inputGain, 24.0f);
            setFloatParameter(processor, VoxlineParameterIDs::drive, 100.0f);
            setFloatParameter(processor, VoxlineParameterIDs::outputGain, 24.0f);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                    buffer.setSample(channel, sample, 2.0f);

            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);

            auto peak = 0.0f;
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

            expect(peak <= 1.0f);
        }

        beginTest("Advanced de-esser parameters audibly control sibilant content");

        {
            VoxlineAudioProcessor dryProcessor;
            VoxlineAudioProcessor deEssProcessor;
            dryProcessor.prepareToPlay(48000.0, 512);
            deEssProcessor.prepareToPlay(48000.0, 512);

            setFloatParameter(dryProcessor, VoxlineParameterIDs::smooth, 0.0f);
            setFloatParameter(deEssProcessor, VoxlineParameterIDs::smooth, 100.0f);
            setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssFreq, 4000.0f);
            setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssThreshold, -40.0f);
            setFloatParameter(deEssProcessor, VoxlineParameterIDs::deEssRange, 12.0f);
            deEssProcessor.getAPVTS().getParameter(VoxlineParameterIDs::deEssMode)->setValueNotifyingHost(1.0f);

            juce::AudioBuffer<float> dry(2, 512);
            for (int channel = 0; channel < dry.getNumChannels(); ++channel)
                for (int sample = 0; sample < dry.getNumSamples(); ++sample)
                    dry.setSample(channel, sample, 0.3f * std::sin(
                        juce::MathConstants<float>::twoPi * 8000.0f * static_cast<float>(sample) / 48000.0f));

            juce::AudioBuffer<float> treated;
            treated.makeCopyOf(dry);
            juce::MidiBuffer midi;
            dryProcessor.processBlock(dry, midi);
            deEssProcessor.processBlock(treated, midi);

            const auto difference = averageAbsoluteDifference(dry, treated);
            expect(difference > 0.001f);
            expect(deEssProcessor.deEssReduction.load() > 1.0f);
        }
    }
};

static VoxlineTests voxlineTests;

int renderEditorScreenshot(const juce::String& outputPath, bool showAdvanced)
{
    VoxlineAudioProcessor processor;
    processor.prepareToPlay(48000.0, 2048);
    juce::AudioBuffer<float> previewAudio(2, 2048);
    juce::MidiBuffer previewMidi;
    for (int pass = 0; pass < 4; ++pass)
    {
        fillTestSignal(previewAudio, 48000.0);
        processor.processBlock(previewAudio, previewMidi);
    }
    VoxlineAudioProcessorEditor editor(processor);

    if (showAdvanced)
        editor.setAdvancedOpen(true);

    juce::Image image(juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics graphics(image);
    editor.paintEntireComponent(graphics, true);

    auto file = juce::File(outputPath).getFullPathName().isEmpty()
                    ? juce::File::getCurrentWorkingDirectory().getChildFile("voxline-editor-screenshot.png")
                    : juce::File(outputPath);

    file.deleteFile();
    juce::PNGImageFormat png;

    if (auto stream = file.createOutputStream())
        if (png.writeImageToStream(image, *stream))
            return 0;

    return 1;
}
} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiScope;

    if (argc >= 2 && (juce::String(argv[1]) == "--render-editor"
                      || juce::String(argv[1]) == "--render-editor-advanced"))
        return renderEditorScreenshot(argc >= 3 ? juce::String(argv[2]) : juce::String(),
                                      juce::String(argv[1]) == "--render-editor-advanced");

    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
