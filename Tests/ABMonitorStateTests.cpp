#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/Parameters/ParameterRegistry.h"
#include "../Source/State/ABState.h"
#include "../Source/State/MonitorState.h"

#include <vector>

namespace
{
class StateTestProcessor final : public juce::AudioProcessor
{
public:
    StateTestProcessor()
        : parameters(*this, nullptr, "VOXLINEState",
                     createVoxlineParameterLayout()) {}

    const juce::String getName() const override { return "StateTest"; }
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

void setPlain(juce::AudioProcessorValueTreeState& state,
              const char* id, float value)
{
    if (auto* parameter = state.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

float getPlain(juce::AudioProcessorValueTreeState& state, const char* id)
{
    if (auto* parameter = state.getParameter(id))
        return parameter->convertFrom0to1(parameter->getValue());
    return std::numeric_limits<float>::quiet_NaN();
}

int savedSoundParameterCount()
{
    auto count = 0;
    for (const auto& spec : Voxline::parameterRegistry())
        if (spec.saveInAb)
            ++count;
    return count;
}

std::vector<float> setSavedSoundNormalised(
    juce::AudioProcessorValueTreeState& state, float value)
{
    std::vector<float> result;
    for (const auto& spec : Voxline::parameterRegistry())
    {
        if (! spec.saveInAb)
            continue;

        auto* parameter = state.getParameter(spec.id);
        if (parameter != nullptr)
        {
            const auto plain =
                parameter->getNormalisableRange().convertFrom0to1(value);
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(plain));
            result.push_back(parameter->getValue());
        }
    }
    return result;
}

std::vector<float> savedSoundNormalised(
    juce::AudioProcessorValueTreeState& state)
{
    std::vector<float> result;
    for (const auto& spec : Voxline::parameterRegistry())
        if (spec.saveInAb)
            if (auto* parameter = state.getParameter(spec.id))
                result.push_back(parameter->getValue());
    return result;
}

void expectSavedSoundMatches(
    juce::UnitTest& test,
    juce::AudioProcessorValueTreeState& state,
    const std::vector<float>& expected)
{
    const auto actual = savedSoundNormalised(state);
    test.expectEquals(actual.size(), expected.size());
    for (size_t index = 0;
         index < juce::jmin(actual.size(), expected.size()); ++index)
        test.expectWithinAbsoluteError(
            actual[index], expected[index], 1.0e-5f,
            "sound parameter " + juce::String(index));
}

class ABMonitorStateTests final : public juce::UnitTest
{
public:
    ABMonitorStateTests()
        : juce::UnitTest("A B and monitor state", "VOXLINE") {}

    void runTest() override
    {
        beginTest("A and B independently round-trip registered sound");
        {
            StateTestProcessor processor;
            VoxlineState::AbStateManager manager(processor.parameters);
            manager.initialiseFromCurrentSound();

            const auto expectedA =
                setSavedSoundNormalised(processor.parameters, 0.2f);
            manager.captureActiveSlot();
            manager.select(VoxlineState::AbSlot::b);
            const auto expectedB =
                setSavedSoundNormalised(processor.parameters, 0.8f);
            manager.captureActiveSlot();

            const auto saved = manager.toValueTree();
            setSavedSoundNormalised(processor.parameters, 0.5f);
            manager.restore(saved);
            expect(manager.activeSlot() == VoxlineState::AbSlot::b);
            expectSavedSoundMatches(*this, processor.parameters, expectedB);

            manager.select(VoxlineState::AbSlot::a);
            expectSavedSoundMatches(*this, processor.parameters, expectedA);
            manager.select(VoxlineState::AbSlot::b);
            expectSavedSoundMatches(*this, processor.parameters, expectedB);
        }

        beginTest("slot selection leaves utility and retired values alone");
        {
            StateTestProcessor processor;
            VoxlineState::AbStateManager manager(processor.parameters);
            manager.initialiseFromCurrentSound();
            manager.select(VoxlineState::AbSlot::b);

            setPlain(processor.parameters, VoxlineParameterIDs::bypass, 1.0f);
            setPlain(processor.parameters, VoxlineParameterIDs::autoGain,
                     0.0f);
            manager.select(VoxlineState::AbSlot::a);

            expectWithinAbsoluteError(
                getPlain(processor.parameters, VoxlineParameterIDs::bypass),
                1.0f, 0.001f);
            expectWithinAbsoluteError(
                getPlain(processor.parameters, VoxlineParameterIDs::autoGain),
                0.0f, 0.001f);
        }

        beginTest("serialised A B state contains only two sound snapshots");
        {
            StateTestProcessor processor;
            VoxlineState::AbStateManager manager(processor.parameters);
            manager.initialiseFromCurrentSound();
            const auto tree = manager.toValueTree();

            expect(tree.hasType("AB_STATE"));
            expectEquals(tree.getProperty("active").toString(),
                         juce::String("A"));
            expectEquals(tree.getNumChildren(), 2);
            for (const auto& slot : tree)
            {
                expectEquals(slot.getNumChildren(),
                             savedSoundParameterCount());
                for (const auto& parameter : slot)
                {
                    const auto* spec = Voxline::findParameterSpec(
                        parameter.getProperty("id").toString());
                    expect(spec != nullptr);
                    if (spec != nullptr)
                        expect(spec->saveInAb);
                }
            }
        }

        beginTest("missing or corrupt snapshots reset both slots safely");
        {
            StateTestProcessor processor;
            VoxlineState::AbStateManager manager(processor.parameters);
            setPlain(processor.parameters, VoxlineParameterIDs::inputGain,
                     4.0f);
            manager.initialiseFromCurrentSound();

            juce::ValueTree incomplete("AB_STATE");
            incomplete.setProperty("active", "B", nullptr);
            incomplete.appendChild(juce::ValueTree("SLOT_A"), nullptr);
            manager.restore(incomplete);
            expect(manager.activeSlot() == VoxlineState::AbSlot::a);
            expectWithinAbsoluteError(
                getPlain(processor.parameters,
                         VoxlineParameterIDs::inputGain),
                4.0f, 0.001f);

            setPlain(processor.parameters, VoxlineParameterIDs::inputGain,
                     2.0f);
            manager.select(VoxlineState::AbSlot::b);
            expectWithinAbsoluteError(
                getPlain(processor.parameters,
                         VoxlineParameterIDs::inputGain),
                4.0f, 0.001f);
        }

        beginTest("monitor modes are exclusive and invalid bands clear");
        {
            VoxlineState::MonitorState monitor;
            expect(monitor.mode() == VoxlineState::MonitorMode::none);
            expectEquals(monitor.eqBand(), -1);

            monitor.setEqBandSolo(5);
            expect(monitor.mode()
                   == VoxlineState::MonitorMode::eqBandSolo);
            expectEquals(monitor.eqBand(), 5);

            monitor.setDeEssListen(true);
            expect(monitor.mode()
                   == VoxlineState::MonitorMode::deEssListenS);
            expectEquals(monitor.eqBand(), -1);

            monitor.setDeEssListen(false);
            expect(monitor.mode() == VoxlineState::MonitorMode::none);
            monitor.setEqBandSolo(6);
            expect(monitor.mode() == VoxlineState::MonitorMode::none);
            expectEquals(monitor.eqBand(), -1);
        }
    }
};

ABMonitorStateTests abMonitorStateTests;
} // namespace
