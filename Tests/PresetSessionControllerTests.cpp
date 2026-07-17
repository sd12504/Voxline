#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/State/MonitorState.h"
#include "../Source/State/PresetSessionController.h"
#include "../Source/State/UserPresetLibrary.h"

namespace
{
class ScopedPresetDirectory
{
public:
    ScopedPresetDirectory()
        : directory(juce::File::getSpecialLocation(
                        juce::File::tempDirectory)
                        .getNonexistentChildFile(
                            "voxline-session-tests", {}, true))
    {
        directory.createDirectory();
    }
    ~ScopedPresetDirectory() { directory.deleteRecursively(); }
    juce::File directory;
};

class SessionTestProcessor final : public juce::AudioProcessor
{
public:
    SessionTestProcessor()
        : parameters(*this, nullptr, "VOXLINEState",
                     createVoxlineParameterLayout()) {}

    const juce::String getName() const override { return "SessionTest"; }
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

class PresetSessionControllerTests final : public juce::UnitTest
{
public:
    PresetSessionControllerTests()
        : juce::UnitTest("Preset session controller", "VOXLINE") {}

    void runTest() override
    {
        beginTest("initial presentation is an unedited Untitled session");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            const auto presentation = controller.presentation();
            expectEquals(presentation.currentName,
                         juce::String("Untitled"));
            expect(! presentation.edited);
            expect(presentation.names.isEmpty());
        }

        beginTest("only registered sound changes mark the preset edited");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 3.0f);
            controller.onParameterChanged();
            expect(controller.isEdited());

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 0.0f);
            controller.onParameterChanged();
            expect(! controller.isEdited());

            setPlain(processor.parameters,
                     VoxlineParameterIDs::bypass, 1.0f);
            controller.onParameterChanged();
            expect(! controller.isEdited());
            setPlain(processor.parameters,
                     VoxlineParameterIDs::autoGain, 0.0f);
            controller.onParameterChanged();
            expect(! controller.isEdited());

            monitor.setEqBandSolo(2);
            expect(! controller.isEdited());
        }

        beginTest("Save As and load reset edited and clear monitor");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 5.0f);
            controller.onParameterChanged();
            expect(controller.saveAs("One").wasOk());
            expect(! controller.isEdited());

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, -4.0f);
            controller.onParameterChanged();
            expect(controller.saveAs("Two").wasOk());
            expectEquals(controller.presentation().currentName,
                         juce::String("Two"));

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 8.0f);
            controller.onParameterChanged();
            monitor.setDeEssListen(true);
            expect(controller.select(
                       "One", VoxlineState::UnsavedAction::discard)
                       .wasOk());
            expectWithinAbsoluteError(
                getPlain(processor.parameters,
                         VoxlineParameterIDs::inputGain),
                5.0f, 0.001f);
            expect(! controller.isEdited());
            expect(monitor.mode() == VoxlineState::MonitorMode::none);
        }

        beginTest("unsaved save discard and cancel are deterministic");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 1.0f);
            controller.onParameterChanged();
            expect(controller.saveAs("One").wasOk());
            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 2.0f);
            controller.onParameterChanged();
            expect(controller.saveAs("Two").wasOk());
            expect(controller.select(
                       "One", VoxlineState::UnsavedAction::discard)
                       .wasOk());

            setPlain(processor.parameters,
                     VoxlineParameterIDs::inputGain, 6.0f);
            controller.onParameterChanged();
            expect(controller.select(
                       "Two", VoxlineState::UnsavedAction::cancel)
                       .failed());
            expectEquals(controller.presentation().currentName,
                         juce::String("One"));
            expectWithinAbsoluteError(
                getPlain(processor.parameters,
                         VoxlineParameterIDs::inputGain),
                6.0f, 0.001f);

            expect(controller.select(
                       "Two", VoxlineState::UnsavedAction::save)
                       .wasOk());
            expect(controller.select(
                       "One", VoxlineState::UnsavedAction::discard)
                       .wasOk());
            expectWithinAbsoluteError(
                getPlain(processor.parameters,
                         VoxlineParameterIDs::inputGain),
                6.0f, 0.001f);
        }

        beginTest("relative selection wraps through user presets only");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            expect(controller.saveAs("Alpha").wasOk());
            expect(controller.saveAs("Beta").wasOk());
            expect(controller.selectRelative(
                       1, VoxlineState::UnsavedAction::discard)
                       .wasOk());
            expectEquals(controller.presentation().currentName,
                         juce::String("Alpha"));
            expect(controller.selectRelative(
                       -1, VoxlineState::UnsavedAction::discard)
                       .wasOk());
            expectEquals(controller.presentation().currentName,
                         juce::String("Beta"));
        }

        beginTest("rename delete and lifecycle hooks update presentation safely");
        {
            ScopedPresetDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            SessionTestProcessor processor;
            VoxlineState::MonitorState monitor;
            VoxlineState::PresetSessionController controller(
                library, processor.parameters, monitor);

            expect(controller.saveAs("Original").wasOk());
            expect(controller.renameCurrent("Renamed").wasOk());
            expectEquals(controller.presentation().currentName,
                         juce::String("Renamed"));

            monitor.setEqBandSolo(1);
            controller.onAbChanged();
            expect(monitor.mode() == VoxlineState::MonitorMode::none);
            monitor.setDeEssListen(true);
            controller.onClose();
            expect(monitor.mode() == VoxlineState::MonitorMode::none);

            expect(controller.remove("Renamed").wasOk());
            expectEquals(controller.presentation().currentName,
                         juce::String("Untitled"));
            expect(controller.presentation().names.isEmpty());
        }
    }
};

PresetSessionControllerTests presetSessionControllerTests;
} // namespace
