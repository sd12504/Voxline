#include <JuceHeader.h>

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

namespace
{
class VoxlineTests final : public juce::UnitTest
{
public:
    VoxlineTests()
        : juce::UnitTest("VOXLINE phases 1 and 2", "VOXLINE")
    {
    }

    void runTest() override
    {
        beginTest("processor exposes the full Phase 1 parameter set");

        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 12);
        }

        beginTest("processor state round-trips parameter values");

        {
            VoxlineAudioProcessor sourceProcessor;
            expectEquals(sourceProcessor.getParameters().size(), 12);

            auto* firstParam = sourceProcessor.getParameters()[0];
            firstParam->setValueNotifyingHost(1.0f);

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            expect(state.getSize() > 0);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto* restoredFirstParam = restoredProcessor.getParameters()[0];
            expectWithinAbsoluteError(restoredFirstParam->getValue(), 1.0f, 0.001f);
        }

        beginTest("debug editor uses the Phase 2 fixed size and creates real controls");

        {
            VoxlineAudioProcessor processor;
            VoxlineAudioProcessorEditor editor(processor);

            expectEquals(editor.getWidth(), 1100);
            expectEquals(editor.getHeight(), 760);

            int sliderCount = 0;
            int buttonCount = 0;

            for (int i = 0; i < editor.getNumChildComponents(); ++i)
            {
                const auto* child = editor.getChildComponent(i);

                if (dynamic_cast<const juce::Slider*>(child) != nullptr)
                    ++sliderCount;

                if (dynamic_cast<const juce::Button*>(child) != nullptr)
                    ++buttonCount;
            }

            expectEquals(sliderCount, 9);
            expectEquals(buttonCount, 3);
        }
    }
};

static VoxlineTests voxlineTests;
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI guiScope;
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
