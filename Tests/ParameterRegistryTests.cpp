#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/Parameters/ParameterRegistry.h"

namespace
{
enum class ParameterKind
{
    floatParameter,
    boolParameter,
    intParameter,
    choiceParameter
};

struct LegacyParameterDescriptor
{
    const char* id;
    ParameterKind kind;
    float rangeStart;
    float rangeEnd;
    float interval;
    float defaultValue;
    int numSteps;
};

constexpr auto continuousSteps = 0x7fffffff;

const std::array<LegacyParameterDescriptor, 51> legacyDescriptors {{
    {VoxlineParameterIDs::inputGain, ParameterKind::floatParameter, -24.0f, 24.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::autoGain, ParameterKind::boolParameter, 0.0f, 1.0f, 1.0f, 1.0f, 2},
    {VoxlineParameterIDs::polish, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 65.0f, continuousSteps},
    {VoxlineParameterIDs::body, ParameterKind::floatParameter, -12.0f, 12.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::clarity, ParameterKind::floatParameter, -12.0f, 12.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::air, ParameterKind::floatParameter, -12.0f, 12.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::smooth, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 32.0f, continuousSteps},
    {VoxlineParameterIDs::comp, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 42.0f, continuousSteps},
    {VoxlineParameterIDs::drive, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 18.0f, continuousSteps},
    {VoxlineParameterIDs::outputGain, ParameterKind::floatParameter, -24.0f, 24.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::bypass, ParameterKind::boolParameter, 0.0f, 1.0f, 1.0f, 0.0f, 2},
    {VoxlineParameterIDs::cleanMode, ParameterKind::boolParameter, 0.0f, 1.0f, 1.0f, 0.0f, 2},
    {VoxlineParameterIDs::listen, ParameterKind::boolParameter, 0.0f, 1.0f, 1.0f, 0.0f, 2},
    {VoxlineParameterIDs::spaceAmount, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::spaceType, ParameterKind::intParameter, 0.0f, 2.0f, 1.0f, 0.0f, 3},
    {VoxlineParameterIDs::spaceTime, ParameterKind::floatParameter, 40.0f, 2000.0f, 1.0f, 1200.0f, continuousSteps},
    {VoxlineParameterIDs::spacePreDelay, ParameterKind::floatParameter, 0.0f, 120.0f, 1.0f, 28.0f, continuousSteps},
    {VoxlineParameterIDs::spaceWidth, ParameterKind::floatParameter, 0.0f, 200.0f, 1.0f, 135.0f, continuousSteps},
    {VoxlineParameterIDs::spaceTone, ParameterKind::floatParameter, -100.0f, 100.0f, 1.0f, 12.0f, continuousSteps},
    {VoxlineParameterIDs::spaceDecay, ParameterKind::floatParameter, 0.1f, 2.5f, 0.01f, 1.6f, continuousSteps},
    {VoxlineParameterIDs::spaceDucking, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 42.0f, continuousSteps},
    {VoxlineParameterIDs::hpfFreq, ParameterKind::floatParameter, 20.0f, 300.0f, 1.0f, 80.0f, continuousSteps},
    {VoxlineParameterIDs::hpfSlope, ParameterKind::choiceParameter, 0.0f, 3.0f, 1.0f, 1.0f, 4},
    {VoxlineParameterIDs::mudAmount, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::eqEnabled, ParameterKind::boolParameter, 0.0f, 1.0f, 1.0f, 1.0f, 2},
    {VoxlineParameterIDs::lowFreq, ParameterKind::floatParameter, 80.0f, 250.0f, 1.0f, 160.0f, continuousSteps},
    {VoxlineParameterIDs::lowGain, ParameterKind::floatParameter, -6.0f, 6.0f, 0.1f, 1.5f, continuousSteps},
    {VoxlineParameterIDs::lowQ, ParameterKind::floatParameter, 0.4f, 2.0f, 0.05f, 0.8f, continuousSteps},
    {VoxlineParameterIDs::mudFreq, ParameterKind::floatParameter, 200.0f, 700.0f, 1.0f, 350.0f, continuousSteps},
    {VoxlineParameterIDs::mudGain, ParameterKind::floatParameter, -12.0f, 12.0f, 0.1f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::mudQ, ParameterKind::floatParameter, 0.5f, 3.0f, 0.05f, 1.1f, continuousSteps},
    {VoxlineParameterIDs::presFreq, ParameterKind::floatParameter, 1000.0f, 5000.0f, 10.0f, 2500.0f, continuousSteps},
    {VoxlineParameterIDs::presGain, ParameterKind::floatParameter, -3.0f, 6.0f, 0.1f, 2.0f, continuousSteps},
    {VoxlineParameterIDs::presQ, ParameterKind::floatParameter, 0.5f, 3.0f, 0.05f, 1.0f, continuousSteps},
    {VoxlineParameterIDs::airFreq, ParameterKind::floatParameter, 6000.0f, 16000.0f, 100.0f, 10000.0f, continuousSteps},
    {VoxlineParameterIDs::airGain, ParameterKind::floatParameter, -3.0f, 6.0f, 0.1f, 1.5f, continuousSteps},
    {VoxlineParameterIDs::airQ, ParameterKind::floatParameter, 0.4f, 2.0f, 0.05f, 0.7f, continuousSteps},
    {VoxlineParameterIDs::lpfFreq, ParameterKind::floatParameter, 8000.0f, 20000.0f, 100.0f, 18000.0f, continuousSteps},
    {VoxlineParameterIDs::lpfSlope, ParameterKind::choiceParameter, 0.0f, 1.0f, 1.0f, 0.0f, 2},
    {VoxlineParameterIDs::compThreshold, ParameterKind::floatParameter, -40.0f, 0.0f, 0.1f, -18.0f, continuousSteps},
    {VoxlineParameterIDs::compRatio, ParameterKind::floatParameter, 1.0f, 10.0f, 0.1f, 3.0f, continuousSteps},
    {VoxlineParameterIDs::compAttack, ParameterKind::floatParameter, 0.5f, 100.0f, 0.5f, 15.0f, continuousSteps},
    {VoxlineParameterIDs::compRelease, ParameterKind::floatParameter, 20.0f, 500.0f, 1.0f, 80.0f, continuousSteps},
    {VoxlineParameterIDs::compMix, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 100.0f, continuousSteps},
    {VoxlineParameterIDs::deEssFreq, ParameterKind::floatParameter, 3000.0f, 12000.0f, 10.0f, 6500.0f, continuousSteps},
    {VoxlineParameterIDs::deEssThreshold, ParameterKind::floatParameter, -40.0f, 0.0f, 0.1f, -18.0f, continuousSteps},
    {VoxlineParameterIDs::deEssRange, ParameterKind::floatParameter, 0.0f, 12.0f, 0.1f, 6.0f, continuousSteps},
    {VoxlineParameterIDs::deEssMode, ParameterKind::choiceParameter, 0.0f, 1.0f, 1.0f, 0.0f, 2},
    {VoxlineParameterIDs::driveTone, ParameterKind::floatParameter, -100.0f, 100.0f, 1.0f, 0.0f, continuousSteps},
    {VoxlineParameterIDs::driveMix, ParameterKind::floatParameter, 0.0f, 100.0f, 1.0f, 70.0f, continuousSteps},
    {VoxlineParameterIDs::driveCharacter, ParameterKind::choiceParameter, 0.0f, 2.0f, 1.0f, 1.0f, 3}
}};

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

ParameterKind parameterKind(const juce::RangedAudioParameter& parameter)
{
    if (dynamic_cast<const juce::AudioParameterFloat*>(&parameter) != nullptr)
        return ParameterKind::floatParameter;
    if (dynamic_cast<const juce::AudioParameterBool*>(&parameter) != nullptr)
        return ParameterKind::boolParameter;
    if (dynamic_cast<const juce::AudioParameterInt*>(&parameter) != nullptr)
        return ParameterKind::intParameter;
    if (dynamic_cast<const juce::AudioParameterChoice*>(&parameter) != nullptr)
        return ParameterKind::choiceParameter;

    jassertfalse;
    return ParameterKind::floatParameter;
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

        beginTest("the first fifty-one Host descriptors remain frozen");
        expectEquals(processor.getParameters().size(), 67);
        for (size_t index = 0; index < legacyDescriptors.size(); ++index)
        {
            const auto& expected = legacyDescriptors[index];
            auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
                processor.getParameters()[static_cast<int>(index)]);
            expect(parameter != nullptr, expected.id);
            if (parameter == nullptr)
                continue;

            expectEquals(parameter->paramID, juce::String(expected.id));
            expect(parameterKind(*parameter) == expected.kind, expected.id);
            const auto& range = parameter->getNormalisableRange();
            expectWithinAbsoluteError(range.start, expected.rangeStart, 0.0001f, expected.id);
            expectWithinAbsoluteError(range.end, expected.rangeEnd, 0.0001f, expected.id);
            expectWithinAbsoluteError(range.interval, expected.interval, 0.0001f, expected.id);
            expectWithinAbsoluteError(plainDefault(*parameter), expected.defaultValue, 0.0001f, expected.id);
            expectEquals(parameter->getNumSteps(), expected.numSteps, expected.id);
        }

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
        const std::array<const char*, 16> appendedIds {
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
            VoxlineParameterIDs::spaceMonoSafety,
            VoxlineParameterIDs::spaceMode,
            VoxlineParameterIDs::spaceSlapTime
        };
        expectEquals(processor.getParameters().size(), 67);
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

        beginTest("new space mode exposes five choices and defaults to Plate");
        auto* spaceMode = dynamic_cast<juce::AudioParameterChoice*>(
            state.getParameter(VoxlineParameterIDs::spaceMode));
        expect(spaceMode != nullptr);
        if (spaceMode != nullptr)
        {
            const juce::StringArray expectedModes{"Room", "Plate", "Hall", "Slap", "Width"};
            expectEquals(spaceMode->choices.size(), expectedModes.size());
            for (int index = 0; index < expectedModes.size(); ++index)
                expectEquals(spaceMode->choices[index], expectedModes[index]);
            expectEquals(static_cast<int>(plainDefault(*spaceMode)), 1);
        }

        beginTest("new slap time has the approved descriptor");
        auto* slapTime = state.getParameter(VoxlineParameterIDs::spaceSlapTime);
        expect(slapTime != nullptr);
        if (slapTime != nullptr)
        {
            const auto& range = slapTime->getNormalisableRange();
            expectWithinAbsoluteError(range.start, 40.0f, 0.0001f);
            expectWithinAbsoluteError(range.end, 250.0f, 0.0001f);
            expectWithinAbsoluteError(range.interval, 1.0f, 0.0001f);
            expectWithinAbsoluteError(plainDefault(*slapTime), 120.0f, 0.0001f);
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

        const std::array<const char*, 10> retiredIds {
            VoxlineParameterIDs::autoGain,
            VoxlineParameterIDs::cleanMode,
            VoxlineParameterIDs::listen,
            VoxlineParameterIDs::mudAmount,
            VoxlineParameterIDs::lowGain,
            VoxlineParameterIDs::presGain,
            VoxlineParameterIDs::airGain,
            VoxlineParameterIDs::compThreshold,
            VoxlineParameterIDs::spaceType,
            VoxlineParameterIDs::spaceTime
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
        source.appendChild(parameterNode(VoxlineParameterIDs::spaceType, 1.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::spaceTime, 140.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::spaceMode, 3.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::spaceSlapTime, 140.0f), nullptr);
        source.appendChild(parameterNode(VoxlineParameterIDs::bypass, 1.0f), nullptr);
        source.appendChild(parameterNode("futureParameter", 0.5f), nullptr);

        const auto copied = Voxline::copyRegisteredSoundState(source);
        expect(copied.hasType(source.getType()));
        expectEquals(copied.getNumProperties(), 0);
        expectEquals(copied.getNumChildren(), 3);
        if (copied.getNumChildren() == 3)
        {
            expectEquals(copied.getChild(0).getProperty("id").toString(),
                         juce::String(VoxlineParameterIDs::body));
            expectEquals(copied.getChild(1).getProperty("id").toString(),
                         juce::String(VoxlineParameterIDs::spaceMode));
            expectEquals(copied.getChild(2).getProperty("id").toString(),
                         juce::String(VoxlineParameterIDs::spaceSlapTime));
        }
    }
};

ParameterRegistryTests parameterRegistryTests;
} // namespace
