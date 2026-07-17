#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterRegistry.h"
#include "../Source/PluginProcessor.h"
#include "../Source/State/StateMigration.h"

#include <cmath>

namespace
{
juce::MemoryBlock binary(const juce::ValueTree& state)
{
    juce::MemoryBlock result;
    if (auto xml = state.createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, result);
    return result;
}

juce::ValueTree decode(const juce::MemoryBlock& bytes)
{
    const auto xml = juce::AudioProcessor::getXmlFromBinary(
        bytes.getData(), static_cast<int>(bytes.getSize()));
    return xml != nullptr ? juce::ValueTree::fromXml(*xml)
                          : juce::ValueTree();
}

juce::ValueTree fixture(const juce::String& name)
{
    auto directory = juce::File::getCurrentWorkingDirectory();
    for (auto depth = 0; depth < 12; ++depth)
    {
        const auto file =
            directory.getChildFile("Tests")
                .getChildFile("Fixtures")
                .getChildFile("State")
                .getChildFile(name);
        if (const auto xml = juce::XmlDocument::parse(file))
            return juce::ValueTree::fromXml(*xml);
        directory = directory.getParentDirectory();
    }
    return {};
}

juce::ValueTree parameterNode(const char* id, float value)
{
    juce::ValueTree node("PARAM");
    node.setProperty("id", id, nullptr);
    node.setProperty("value", value, nullptr);
    return node;
}

juce::ValueTree findParameter(const juce::ValueTree& state,
                              const char* id)
{
    for (const auto& child : state)
        if (child.getProperty("id").toString() == id)
            return child;
    return {};
}

void setPlain(VoxlineAudioProcessor& processor,
              const char* id, float value)
{
    if (auto* parameter = processor.getAPVTS().getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

float getPlain(VoxlineAudioProcessor& processor, const char* id)
{
    const auto* value = processor.getAPVTS().getRawParameterValue(id);
    return value != nullptr ? value->load()
                            : std::numeric_limits<float>::quiet_NaN();
}

void configureChannels(VoxlineAudioProcessor& processor,
                       juce::UnitTest& test)
{
    auto layout = processor.getBusesLayout();
    layout.getChannelSet(true, 0) = juce::AudioChannelSet::stereo();
    layout.getChannelSet(false, 0) = juce::AudioChannelSet::stereo();
    test.expect(processor.setBusesLayout(layout));
}

void fillMonitorSignal(juce::AudioBuffer<float>& buffer,
                       int64_t startSample)
{
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto position =
                static_cast<double>(startSample + sample) / 48000.0;
            buffer.setSample(
                channel, sample,
                0.12f * static_cast<float>(std::sin(
                            juce::MathConstants<double>::twoPi
                            * 160.0 * position))
                    + 0.08f * static_cast<float>(std::sin(
                            juce::MathConstants<double>::twoPi
                            * 7000.0 * position)));
        }
}

float maximumDifference(const juce::AudioBuffer<float>& left,
                        const juce::AudioBuffer<float>& right)
{
    auto result = 0.0f;
    for (auto channel = 0; channel < left.getNumChannels(); ++channel)
        for (auto sample = 0; sample < left.getNumSamples(); ++sample)
            result = juce::jmax(
                result,
                std::abs(left.getSample(channel, sample)
                         - right.getSample(channel, sample)));
    return result;
}

class HostStateIntegrationTests final : public juce::UnitTest
{
public:
    HostStateIntegrationTests()
        : juce::UnitTest("Host state integration", "VOXLINE") {}

    void runTest() override
    {
        beginTest("host state contains parameters and persisted A B snapshots");
        {
            VoxlineAudioProcessor processor;
            juce::MemoryBlock bytes;
            processor.getStateInformation(bytes);
            const auto state = decode(bytes);
            const auto parameters = state.getChildWithName("PARAMETERS");
            const auto ab = state.getChildWithName("AB_STATE");

            expect(state.hasType("VOXLINEState"));
            expectEquals(
                static_cast<int>(state.getProperty("schemaVersion")),
                VoxlineState::currentSchemaVersion);
            expect(parameters.isValid());
            expect(ab.isValid());
            expectEquals(ab.getNumChildren(), 2);
            expect(! findParameter(
                        parameters, VoxlineParameterIDs::autoGain)
                        .isValid());
            expect(! findParameter(
                        parameters, VoxlineParameterIDs::spaceType)
                        .isValid());
        }

        beginTest("full Host round-trip restores both A B slots and active slot");
        {
            VoxlineAudioProcessor source;
            auto& sourceAb = source.getAbState();
            setPlain(source, VoxlineParameterIDs::inputGain, 5.0f);
            sourceAb.captureActiveSlot();
            sourceAb.select(VoxlineState::AbSlot::b);
            setPlain(source, VoxlineParameterIDs::inputGain, -4.0f);
            sourceAb.captureActiveSlot();

            juce::MemoryBlock bytes;
            source.getStateInformation(bytes);
            VoxlineAudioProcessor restored;
            restored.setStateInformation(
                bytes.getData(), static_cast<int>(bytes.getSize()));

            expect(restored.getAbState().activeSlot()
                   == VoxlineState::AbSlot::b);
            expectWithinAbsoluteError(
                getPlain(restored, VoxlineParameterIDs::inputGain),
                -4.0f, 0.001f);
            restored.getAbState().select(VoxlineState::AbSlot::a);
            expectWithinAbsoluteError(
                getPlain(restored, VoxlineParameterIDs::inputGain),
                5.0f, 0.001f);
        }

        beginTest("sparse legacy states use declared defaults for missing values");
        {
            VoxlineAudioProcessor processor;
            for (const auto& spec : Voxline::parameterRegistry())
                if (auto* parameter =
                        processor.getAPVTS().getParameter(spec.id))
                    parameter->setValueNotifyingHost(1.0f);

            const auto bytes = binary(fixture("v1-percent.xml"));
            processor.setStateInformation(
                bytes.getData(), static_cast<int>(bytes.getSize()));

            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::inputGain),
                2.5f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::body),
                -12.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::air),
                12.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::outputGain),
                0.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::polish),
                65.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor,
                         VoxlineParameterIDs::compSensitivity),
                0.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor, VoxlineParameterIDs::spaceMode),
                0.0f, 0.001f);
        }

        beginTest("corrupt future and invalid A B states are transactional");
        {
            VoxlineAudioProcessor processor;
            setPlain(processor, VoxlineParameterIDs::inputGain, 3.0f);
            processor.getAbState().captureActiveSlot();
            processor.getAbState().select(VoxlineState::AbSlot::b);
            setPlain(processor, VoxlineParameterIDs::inputGain, -2.0f);
            processor.getAbState().captureActiveSlot();

            const auto beforeParameters = processor.getAPVTS().copyState();
            const auto beforeAb = processor.getAbState().toValueTree();

            const std::array<std::byte, 4> corrupt {
                std::byte {0x56}, std::byte {0x4f},
                std::byte {0x58}, std::byte {0x00}};
            processor.setStateInformation(
                corrupt.data(), static_cast<int>(corrupt.size()));

            juce::ValueTree future("VOXLINEState");
            future.setProperty(
                "schemaVersion",
                VoxlineState::currentSchemaVersion + 1, nullptr);
            const auto futureBytes = binary(future);
            processor.setStateInformation(
                futureBytes.getData(),
                static_cast<int>(futureBytes.getSize()));

            juce::MemoryBlock validBytes;
            processor.getStateInformation(validBytes);
            auto invalidParameter = decode(validBytes);
            findParameter(
                invalidParameter.getChildWithName("PARAMETERS"),
                VoxlineParameterIDs::inputGain)
                .setProperty("value", "NaN", nullptr);
            const auto invalidParameterBytes = binary(invalidParameter);
            processor.setStateInformation(
                invalidParameterBytes.getData(),
                static_cast<int>(invalidParameterBytes.getSize()));

            auto invalidAb = decode(validBytes);
            invalidAb.getChildWithName("AB_STATE").removeChild(1, nullptr);
            const auto invalidAbBytes = binary(invalidAb);
            processor.setStateInformation(
                invalidAbBytes.getData(),
                static_cast<int>(invalidAbBytes.getSize()));

            expect(processor.getAPVTS().copyState().isEquivalentTo(
                beforeParameters));
            expect(processor.getAbState().toValueTree().isEquivalentTo(
                beforeAb));
        }

        beginTest("legacy Space automation drives the compatibility bridge");
        {
            juce::ValueTree legacy("VOXLINEState");
            legacy.setProperty("schemaVersion", 2, nullptr);
            for (const auto& parameter : {
                     parameterNode(VoxlineParameterIDs::inputGain, 0.0f),
                     parameterNode(VoxlineParameterIDs::outputGain, 0.0f),
                     parameterNode(VoxlineParameterIDs::polish, 0.0f),
                     parameterNode(VoxlineParameterIDs::smooth, 0.0f),
                     parameterNode(VoxlineParameterIDs::comp, 0.0f),
                     parameterNode(VoxlineParameterIDs::drive, 0.0f),
                     parameterNode(VoxlineParameterIDs::eqEnabled, 0.0f),
                     parameterNode(VoxlineParameterIDs::spaceAmount, 100.0f),
                     parameterNode(VoxlineParameterIDs::spaceType, 0.0f),
                     parameterNode(VoxlineParameterIDs::spaceTime, 1200.0f),
                     parameterNode(VoxlineParameterIDs::spaceFeedback, 0.0f)})
                legacy.appendChild(parameter.createCopy(), nullptr);

            VoxlineAudioProcessor processor;
            configureChannels(processor, *this);
            const auto bytes = binary(legacy);
            processor.setStateInformation(
                bytes.getData(), static_cast<int>(bytes.getSize()));
            setPlain(processor, VoxlineParameterIDs::spaceType, 1.0f);
            setPlain(processor, VoxlineParameterIDs::spaceTime, 80.0f);
            processor.prepareToPlay(48000.0, 512);

            const auto expectedRepeat =
                processor.getLatencySamples()
                + juce::roundToInt(48000.0 * 0.080);
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            auto firstRepeat = -1;
            auto absoluteSample = 0;
            for (auto block = 0; block < 20; ++block)
            {
                buffer.clear();
                if (block == 0)
                    for (auto channel = 0; channel < 2; ++channel)
                        buffer.setSample(channel, 0, 0.5f);
                processor.processBlock(buffer, midi);
                for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
                    if (absoluteSample + sample
                            > processor.getLatencySamples() + 8
                        && std::abs(buffer.getSample(0, sample)) > 1.0e-4f
                        && firstRepeat < 0)
                        firstRepeat = absoluteSample + sample;
                absoluteSample += buffer.getNumSamples();
            }
            expectWithinAbsoluteError(firstRepeat, expectedRepeat, 1);
        }

        beginTest("monitor routing is transient and clears on state load");
        {
            VoxlineAudioProcessor processor;
            processor.getMonitorState().setEqBandSolo(1);
            juce::MemoryBlock bytes;
            processor.getStateInformation(bytes);

            VoxlineAudioProcessor restored;
            restored.getMonitorState().setDeEssListen(true);
            restored.setStateInformation(
                bytes.getData(), static_cast<int>(bytes.getSize()));
            expect(restored.getMonitorState().mode()
                   == VoxlineState::MonitorMode::none);

            const auto state = decode(bytes);
            expect(! state.hasProperty("monitor"));
            expect(! state.getChildWithName("MONITOR_STATE").isValid());
        }

        beginTest("processor routes EQ Solo and Listen S monitor audio");
        {
            VoxlineAudioProcessor normal;
            VoxlineAudioProcessor monitored;
            configureChannels(normal, *this);
            configureChannels(monitored, *this);
            for (auto* processor : {&normal, &monitored})
            {
                setPlain(*processor, VoxlineParameterIDs::inputGain, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::outputGain, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::polish, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::comp, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::drive, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::spaceAmount, 0.0f);
                setPlain(*processor, VoxlineParameterIDs::eqEnabled, 1.0f);
                setPlain(*processor, VoxlineParameterIDs::body, 6.0f);
                processor->prepareToPlay(48000.0, 512);
            }

            monitored.getMonitorState().setEqBandSolo(1);
            juce::AudioBuffer<float> normalBuffer(2, 512);
            juce::AudioBuffer<float> monitoredBuffer(2, 512);
            juce::MidiBuffer midi;
            int64_t position = 0;
            for (auto block = 0; block < 8; ++block)
            {
                fillMonitorSignal(normalBuffer, position);
                monitoredBuffer.makeCopyOf(normalBuffer);
                normal.processBlock(normalBuffer, midi);
                monitored.processBlock(monitoredBuffer, midi);
                position += 512;
            }
            expect(maximumDifference(normalBuffer, monitoredBuffer)
                   > 1.0e-3f);

            monitored.getMonitorState().setDeEssListen(true);
            fillMonitorSignal(normalBuffer, position);
            monitoredBuffer.makeCopyOf(normalBuffer);
            normal.processBlock(normalBuffer, midi);
            monitored.processBlock(monitoredBuffer, midi);
            expect(maximumDifference(normalBuffer, monitoredBuffer)
                   > 1.0e-3f);
        }
    }
};

HostStateIntegrationTests hostStateIntegrationTests;
} // namespace
