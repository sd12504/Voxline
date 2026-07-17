#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/Parameters/ParameterRegistry.h"

namespace
{
class LayoutTestProcessor final : public juce::AudioProcessor
{
public:
    LayoutTestProcessor()
        : parameters(*this, nullptr, "VOXLINEState", createVoxlineParameterLayout()) {}

    const juce::String getName() const override { return "LayoutTestProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState parameters;
};

float plainDefault(const juce::RangedAudioParameter& parameter)
{
    return parameter.getNormalisableRange().convertFrom0to1(parameter.getDefaultValue());
}

juce::ValueTree parameterNode(const char* id, float value)
{
    juce::ValueTree node("PARAM");
    node.setProperty("id", id, nullptr);
    node.setProperty("value", value, nullptr);
    return node;
}

class ParameterRegistryTests final : public juce::UnitTest
{
public:
    ParameterRegistryTests()
        : juce::UnitTest("Parameter registry v3", "VOXLINE") {}

    void runTest() override
    {
        LayoutTestProcessor processor;
        auto& state = processor.parameters;

        beginTest("active EQ gains use plus or minus twelve dB");
        for (const auto* id : {VoxlineParameterIDs::body,
                              VoxlineParameterIDs::clarity,
                              VoxlineParameterIDs::air,
                              VoxlineParameterIDs::mudGain})
        {
            auto* parameter = state.getParameter(id);
            expect(parameter != nullptr, id);
            if (parameter == nullptr)
                continue;

            const auto& range = parameter->getNormalisableRange();
            expectWithinAbsoluteError(range.start, -12.0f, 0.0001f, id);
            expectWithinAbsoluteError(range.end, 12.0f, 0.0001f, id);
            expectWithinAbsoluteError(range.interval, 0.1f, 0.0001f, id);
            expectWithinAbsoluteError(plainDefault(*parameter), 0.0f, 0.0001f, id);
        }

        beginTest("main and advanced EQ share the active gain IDs");
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::body)->role == Voxline::ParameterRole::sound);
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::clarity)->role == Voxline::ParameterRole::sound);
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::air)->role == Voxline::ParameterRole::sound);
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::lowGain)->role == Voxline::ParameterRole::retired);
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::presGain)->role == Voxline::ParameterRole::retired);
        expect(Voxline::findParameterSpec(VoxlineParameterIDs::airGain)->role == Voxline::ParameterRole::retired);

        beginTest("v3 parameters are appended after every legacy parameter");
        const std::array<const char*, 14> appendedIds {
            VoxlineParameterIDs::hpfEnabled,
            VoxlineParameterIDs::lowEnabled,
            VoxlineParameterIDs::mudEnabled,
            VoxlineParameterIDs::presEnabled,
            VoxlineParameterIDs::airEnabled,
            VoxlineParameterIDs::lpfEnabled,
            VoxlineParameterIDs::compSensitivity,
            VoxlineParameterIDs::compMakeup,
            VoxlineParameterIDs::compAutoMakeup,
            VoxlineParameterIDs::driveOutputTrim,
            VoxlineParameterIDs::driveLevelMatch,
            VoxlineParameterIDs::spaceSize,
            VoxlineParameterIDs::spaceFeedback,
            VoxlineParameterIDs::spaceMonoSafety
        };
        expectEquals(processor.getParameters().size(), 65);
        for (size_t index = 0; index < appendedIds.size(); ++index)
        {
            auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
                processor.getParameters()[static_cast<int>(51 + index)]);
            expect(parameter != nullptr);
            if (parameter != nullptr)
                expectEquals(parameter->paramID, juce::String(appendedIds[index]));
        }

        beginTest("v3 defaults match the product contract");
        const auto expectBoolDefault = [this, &state](const char* id, bool expected)
        {
            auto* parameter = state.getParameter(id);
            expect(parameter != nullptr, id);
            if (parameter != nullptr)
                expectWithinAbsoluteError(plainDefault(*parameter), expected ? 1.0f : 0.0f,
                                          0.0001f, id);
        };
        expectBoolDefault(VoxlineParameterIDs::hpfEnabled, false);
        expectBoolDefault(VoxlineParameterIDs::lowEnabled, true);
        expectBoolDefault(VoxlineParameterIDs::mudEnabled, false);
        expectBoolDefault(VoxlineParameterIDs::presEnabled, true);
        expectBoolDefault(VoxlineParameterIDs::airEnabled, true);
        expectBoolDefault(VoxlineParameterIDs::lpfEnabled, false);
        expectBoolDefault(VoxlineParameterIDs::compAutoMakeup, true);
        expectBoolDefault(VoxlineParameterIDs::driveLevelMatch, true);
        expectBoolDefault(VoxlineParameterIDs::spaceMonoSafety, true);

        expectWithinAbsoluteError(
            plainDefault(*state.getParameter(VoxlineParameterIDs::compSensitivity)),
            0.0f, 0.0001f);
        expectWithinAbsoluteError(
            plainDefault(*state.getParameter(VoxlineParameterIDs::compMakeup)),
            0.0f, 0.0001f);
        expectWithinAbsoluteError(
            plainDefault(*state.getParameter(VoxlineParameterIDs::driveOutputTrim)),
            0.0f, 0.0001f);
        expectWithinAbsoluteError(
            plainDefault(*state.getParameter(VoxlineParameterIDs::spaceSize)),
            62.0f, 0.0001f);
        expectWithinAbsoluteError(
            plainDefault(*state.getParameter(VoxlineParameterIDs::spaceFeedback)),
            20.0f, 0.0001f);

        beginTest("space type exposes the five approved modes and defaults to Plate");
        auto* spaceType = dynamic_cast<juce::AudioParameterChoice*>(
            state.getParameter(VoxlineParameterIDs::spaceType));
        expect(spaceType != nullptr);
        if (spaceType != nullptr)
        {
            const juce::StringArray expectedModes{"Room", "Plate", "Hall", "Slap", "Width"};
            expectEquals(spaceType->choices.size(), expectedModes.size());
            for (int index = 0; index < expectedModes.size(); ++index)
                expectEquals(spaceType->choices[index], expectedModes[index]);
            expectEquals(static_cast<int>(plainDefault(*spaceType)), 1);
        }

        beginTest("registry defines persistence rules for every parameter");
        for (auto* rawParameter : processor.getParameters())
        {
            auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(rawParameter);
            expect(parameter != nullptr);
            if (parameter == nullptr)
                continue;

            const auto* spec = Voxline::findParameterSpec(parameter->paramID);
            expect(spec != nullptr, parameter->paramID);
            if (spec != nullptr && spec->role == Voxline::ParameterRole::sound)
            {
                expect(spec->saveInPreset, parameter->paramID);
                expect(spec->saveInAb, parameter->paramID);
            }
        }

        const std::array<const char*, 8> retiredIds {
            VoxlineParameterIDs::autoGain,
            VoxlineParameterIDs::cleanMode,
            VoxlineParameterIDs::listen,
            VoxlineParameterIDs::mudAmount,
            VoxlineParameterIDs::lowGain,
            VoxlineParameterIDs::presGain,
            VoxlineParameterIDs::airGain,
            VoxlineParameterIDs::compThreshold
        };
        for (const auto* id : retiredIds)
        {
            const auto* spec = Voxline::findParameterSpec(id);
            expect(spec != nullptr, id);
            if (spec != nullptr)
            {
                expect(spec->role == Voxline::ParameterRole::retired, id);
                expect(! spec->saveInPreset, id);
                expect(! spec->saveInAb, id);
            }
        }

        const auto* bypass = Voxline::findParameterSpec(VoxlineParameterIDs::bypass);
        expect(bypass != nullptr);
        if (bypass != nullptr)
        {
            expect(bypass->role == Voxline::ParameterRole::utility);
            expect(! bypass->saveInPreset);
            expect(! bypass->saveInAb);
        }
        expect(Voxline::findParameterSpec("unknownParameter") == nullptr);

        beginTest("sound state copies only registered audible parameters");
        juce::ValueTree source("VOXLINEState");
        source.setProperty("uiExpanded", true, nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::body, 2.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::autoGain, 1.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::lowGain, 4.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::bypass, 1.0f), nullptr);
        source.appendChild(parameterNode("futureParameter", 0.5f), nullptr);

        const auto copied = Voxline::copyRegisteredSoundState(source);
        expect(copied.hasType(source.getType()));
        expectEquals(copied.getNumProperties(), 0);
        expectEquals(copied.getNumChildren(), 1);
        if (copied.getNumChildren() == 1)
            expectEquals(copied.getChild(0).getProperty("id").toString(),
                         juce::String(VoxlineParameterIDs::body));
    }
};

ParameterRegistryTests parameterRegistryTests;
} // namespace
