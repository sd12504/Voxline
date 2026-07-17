#include <JuceHeader.h>

#include "../Source/Parameters/ParameterRegistry.h"
#include "../Source/PluginProcessor.h"
#include "../Source/State/StateSchema.h"
#include "TestSupport.h"

#include <vector>

namespace
{
std::vector<float> registeredSoundValues(
    const VoxlineAudioProcessor& processor)
{
    std::vector<float> values;
    for (const auto& spec : Voxline::parameterRegistry())
        if (spec.role == Voxline::ParameterRole::sound)
            if (const auto* parameter =
                    processor.getAPVTS().getParameter(spec.id))
                values.push_back(parameter->getValue());
    return values;
}

class ProcessorContractTests final : public juce::UnitTest
{
public:
    ProcessorContractTests()
        : juce::UnitTest("Processor contract", "VOXLINE") {}

    void runTest() override
    {
        beginTest("processor exposes the full parameter set");
        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 67);

            auto* body = processor.getAPVTS().getParameter(VoxlineParameterIDs::body);
            expect(body != nullptr);
            expectWithinAbsoluteError(body->getNormalisableRange().start, -12.0f, 0.001f);
            expectWithinAbsoluteError(body->getNormalisableRange().end, 12.0f, 0.001f);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::compThreshold) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::deEssFreq) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::driveCharacter) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::spaceTime) != nullptr);
            expect(processor.getAPVTS().getParameter(VoxlineParameterIDs::spaceDucking) != nullptr);
            expectEquals(processor.getNumPrograms(), 1);
            expectEquals(processor.getCurrentProgram(), 0);
            expectEquals(
                processor.getProgramName(0), juce::String("Default"));

            VoxlineTest::setFloatParameter(
                processor, VoxlineParameterIDs::polish, 37.0f);
            const auto beforeProgramCalls =
                registeredSoundValues(processor);
            processor.setCurrentProgram(8);
            processor.changeProgramName(0, "Changed");
            expectEquals(processor.getCurrentProgram(), 0);
            expect(
                registeredSoundValues(processor)
                    == beforeProgramCalls);
            expectEquals(
                processor.getProgramName(0), juce::String("Default"));
            expectWithinAbsoluteError(
                processor.getTailLengthSeconds(),
                0.0,
                1.0e-9);
        }

        beginTest("processor state round-trips parameter values");
        {
            VoxlineAudioProcessor sourceProcessor;
            expectEquals(sourceProcessor.getParameters().size(), 67);

            auto* firstParam = sourceProcessor.getParameters()[0];
            firstParam->setValueNotifyingHost(1.0f);
            VoxlineTest::setFloatParameter(
                sourceProcessor,
                VoxlineParameterIDs::compSensitivity,
                73.0f);
            auto* sourceSpaceMode = sourceProcessor.getAPVTS().getParameter(
                VoxlineParameterIDs::spaceMode);
            expect(sourceSpaceMode != nullptr);
            if (sourceSpaceMode != nullptr)
                sourceSpaceMode->setValueNotifyingHost(
                    sourceSpaceMode->convertTo0to1(3.0f));

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            expect(state.getSize() > 0);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto* restoredFirstParam = restoredProcessor.getParameters()[0];
            expectWithinAbsoluteError(restoredFirstParam->getValue(), 1.0f, 0.001f);
            expectWithinAbsoluteError(
                restoredProcessor.getAPVTS()
                    .getRawParameterValue(
                        VoxlineParameterIDs::compSensitivity)
                    ->load(),
                73.0f,
                0.01f);
            expectWithinAbsoluteError(
                restoredProcessor.getAPVTS()
                    .getRawParameterValue(VoxlineParameterIDs::spaceMode)
                    ->load(),
                3.0f,
                0.001f);
        }

        beginTest("state schema rejects corrupt and wrong-root data");
        {
            VoxlineAudioProcessor processor;
            const auto before = processor.getAPVTS().copyState();

            const std::array<std::byte, 4> corrupt {
                std::byte{0x56}, std::byte{0x4f}, std::byte{0x58}, std::byte{0x00}
            };
            processor.setStateInformation(corrupt.data(), static_cast<int>(corrupt.size()));
            expect(processor.getAPVTS().copyState().isEquivalentTo(before));

            juce::ValueTree wrong("WrongRoot");
            std::unique_ptr<juce::XmlElement> xml(wrong.createXml());
            expect(xml != nullptr);
            if (xml != nullptr)
            {
                juce::MemoryBlock bytes;
                juce::AudioProcessor::copyXmlToBinary(*xml, bytes);
                processor.setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
                expect(processor.getAPVTS().copyState().isEquivalentTo(before));
            }
        }

        beginTest("new state writes a schema version");
        {
            VoxlineAudioProcessor processor;
            juce::MemoryBlock bytes;
            processor.getStateInformation(bytes);
            auto xml = juce::AudioProcessor::getXmlFromBinary(bytes.getData(),
                static_cast<int>(bytes.getSize()));
            expect(xml != nullptr);
            if (xml != nullptr)
            {
                expect(xml->hasAttribute("schemaVersion"));
                expectEquals(
                    xml->getIntAttribute("schemaVersion"),
                    VoxlineState::currentSchemaVersion);
            }
        }

        beginTest("legacy unversioned state still restores");
        {
            VoxlineAudioProcessor processor;
            auto* sourceInputGain = processor.getAPVTS().getParameter(VoxlineParameterIDs::inputGain);
            sourceInputGain->setValueNotifyingHost(1.0f);
            auto legacyState = processor.getAPVTS().copyState();
            legacyState.removeProperty("schemaVersion", nullptr);

            juce::MemoryBlock bytes;
            if (auto xml = legacyState.createXml())
                juce::AudioProcessor::copyXmlToBinary(*xml, bytes);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
            expectWithinAbsoluteError(restoredProcessor.getAPVTS()
                                          .getParameter(VoxlineParameterIDs::inputGain)->getValue(),
                                      1.0f, 0.001f);
        }

        beginTest("Phase 3 bypass returns the dry signal");
        {
            VoxlineAudioProcessor processor;
            processor.prepareToPlay(48000.0, 512);
            VoxlineTest::setBoolParameter(processor, VoxlineParameterIDs::bypass, true);

            juce::AudioBuffer<float> continuousDry(2, 1024);
            VoxlineTest::fillTestSignal(continuousDry, 48000.0);
            juce::AudioBuffer<float> warmupBuffer(2, 512);
            juce::AudioBuffer<float> buffer(2, 512);
            juce::AudioBuffer<float> expectedBuffer(2, 512);
            const auto latency = processor.getLatencySamples();
            for (auto channel = 0; channel < 2; ++channel)
            {
                warmupBuffer.copyFrom(channel, 0, continuousDry, channel, 0, 512);
                buffer.copyFrom(channel, 0, continuousDry, channel, 512, 512);
                for (auto sample = 0; sample < 512; ++sample)
                {
                    const auto sourceSample = 512 + sample - latency;
                    expectedBuffer.setSample(
                        channel,
                        sample,
                        sourceSample >= 0
                            ? continuousDry.getSample(channel, sourceSample)
                            : 0.0f);
                }
            }
            juce::MidiBuffer midi;

            processor.processBlock(warmupBuffer, midi);
            processor.processBlock(buffer, midi);

            expectWithinAbsoluteError(
                VoxlineTest::averageAbsoluteDifference(buffer, expectedBuffer),
                0.0f,
                1.0e-6f);
        }
    }
};

ProcessorContractTests processorContractTests;
}
